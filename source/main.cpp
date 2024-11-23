#include "async_postgres.hpp"

int async_postgres::connection_meta = 0;

#define lua_connection_state()                    \
    lua->GetUserType<async_postgres::Connection>( \
        1, async_postgres::connection_meta)

namespace async_postgres::lua {
    lua_protected_fn(__gc) {
        delete lua_connection_state();
        return 0;
    }

    lua_protected_fn(__index) {
        auto state = lua_connection_state();

        state->lua_table.Push();
        lua->Push(2);
        lua->GetTable(-2);
        if (!lua->IsType(-1, GLua::Type::Nil)) {
            return 1;
        }

        // is it alright if I don't pop previous stack values?

        lua->PushMetaTable(async_postgres::connection_meta);
        lua->Push(2);
        lua->GetTable(-2);

        return 1;
    }

    lua_protected_fn(__newindex) {
        auto state = lua_connection_state();

        state->lua_table.Push();
        lua->Push(2);
        lua->Push(3);
        lua->SetTable(-3);

        auto key = get_string(lua, 2);
        if (key == "on_notify") {
            state->receive_notifications = !lua->IsType(3, GLua::Type::Nil);
        }

        return 1;
    }

    lua_protected_fn(loop) {
        async_postgres::process_pending_connections(lua);

        for (auto* state : async_postgres::connections) {
            if (!state->conn) {
                lua->Msg("[async_postgres] connection is null for %p\n", state);
                continue;
            }

            async_postgres::process_reset(lua, state);
            async_postgres::process_notifications(lua, state);
            async_postgres::process_queries(lua, state);
        }

        return 0;
    }

    lua_protected_fn(connect) {
        lua->CheckType(1, GLua::Type::String);
        lua->CheckType(2, GLua::Type::Function);

        auto url = lua->GetString(1);
        GLua::AutoReference callback(lua, 2);

        async_postgres::connect(lua, url, std::move(callback));

        return 0;
    }

    lua_protected_fn(query) {
        lua->CheckType(1, async_postgres::connection_meta);
        lua->CheckType(2, GLua::Type::String);
        lua->CheckType(3, GLua::Type::Function);

        auto state = lua_connection_state();

        async_postgres::SimpleCommand command = {lua->GetString(2)};
        async_postgres::Query query = {std::move(command)};
        if (!lua->IsType(3, GLua::Type::Nil)) {
            query.callback = GLua::AutoReference(lua, 3);
        }

        state->queries.push(std::move(query));
        return 0;
    }

    lua_protected_fn(queryParams) {
        lua->CheckType(1, async_postgres::connection_meta);
        lua->CheckType(2, GLua::Type::String);
        lua->CheckType(3, GLua::Type::Table);
        lua->CheckType(4, GLua::Type::Function);

        auto state = lua_connection_state();

        async_postgres::ParameterizedCommand command = {lua->GetString(2)};

        lua->Push(3);
        for (int i = 1;; i++) {
            lua->PushNumber(i);
            lua->GetTable(-2);
            if (lua->IsType(-1, GLua::Type::Nil)) {
                lua->Pop(2);
                break;
            }

            auto str = get_string(lua, -1);
            command.values.push_back({str.data(), str.size()});
            lua->Pop(1);
        }

        async_postgres::Query query = {std::move(command)};
        query.callback = GLua::AutoReference(lua, 4);
        state->queries.push(std::move(query));

        return 0;
    }

    lua_protected_fn(reset) {
        lua->CheckType(1, async_postgres::connection_meta);

        GLua::AutoReference callback;
        if (!lua->IsType(2, GLua::Type::Nil)) {
            callback = GLua::AutoReference(lua, 2);
        }

        auto state = lua_connection_state();
        async_postgres::reset(lua, state, std::move(callback));

        return 0;
    }
}  // namespace async_postgres::lua

#define register_lua_fn(name)                      \
    lua->PushCFunction(async_postgres::lua::name); \
    lua->SetField(-2, #name)

void register_connection_mt(GLua::ILuaInterface* lua) {
    async_postgres::connection_meta = lua->CreateMetaTable("PGconn");

    register_lua_fn(__index);
    register_lua_fn(__newindex);
    register_lua_fn(__gc);
    register_lua_fn(query);
    register_lua_fn(reset);

    async_postgres::register_misc_connection_functions(lua);

    lua->Pop();
}

void make_global_table(GLua::ILuaInterface* lua) {
    lua->CreateTable();

    register_lua_fn(connect);

    lua->SetField(GLua::INDEX_GLOBAL, "async_postgres");
}

void register_loop_hook(GLua::ILuaInterface* lua) {
    lua->GetField(GLua::INDEX_GLOBAL, "hook");
    lua->GetField(-1, "Add");
    lua->PushString("Think");
    lua->PushString("async_postgres_loop");
    lua->PushCFunction(async_postgres::lua::loop);
    lua->Call(3, 0);
    lua->Pop();
}

GMOD_MODULE_OPEN() {
    auto lua = reinterpret_cast<GLua::ILuaInterface*>(LUA);

    register_connection_mt(lua);
    make_global_table(lua);
    register_loop_hook(lua);

    return 0;
}

GMOD_MODULE_CLOSE() { return 0; }
