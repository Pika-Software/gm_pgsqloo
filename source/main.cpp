#include <GarrysMod/Lua/Interface.h>

#include "pgsqloo.hpp"

int pgsqloo::connection_meta = 0;

#define lua_connection_state() \
    lua->GetUserType<pgsqloo::Connection>(1, pgsqloo::connection_meta)

lua_protected_fn(gc_connection) {
    delete lua_connection_state();
    return 0;
}

pgsqloo::Connection::~Connection() {
    // remove connection from global list
    // so event loop doesn't try to process it
    pgsqloo::connections.erase(std::find(pgsqloo::connections.begin(),
                                         pgsqloo::connections.end(), this));
}

lua_protected_fn(index_connection) {
    auto state = lua_connection_state();

    state->lua_table.Push();
    lua->Push(2);
    lua->GetTable(-2);
    if (!lua->IsType(-1, GLua::Type::Nil)) {
        return 1;
    }

    // is it alright if I don't pop previous stack values?

    lua->PushMetaTable(pgsqloo::connection_meta);
    lua->Push(2);
    lua->GetTable(-2);

    return 1;
}

lua_protected_fn(newindex_connection) {
    auto state = lua_connection_state();

    state->lua_table.Push();
    lua->Push(2);
    lua->Push(3);
    lua->SetTable(-3);

    return 1;
}

// inline void process_connections(GLua::ILuaInterface* lua) {
//     for (auto it = connections.begin(); it != connections.end();) {
//         auto& event = *it;
//         auto status = PQconnectPoll(event.conn);
//         if (status == PGRES_POLLING_OK || status == PGRES_POLLING_FAILED) {
//             lua->GetField(GLua::INDEX_GLOBAL, "ErrorNoHaltWithStack");
//             lua->ReferencePush(event.callback);
//             if (status == PGRES_POLLING_OK) {
//                 lua->PushBool(true);
//                 lua->PushUserType(event.conn, connection_meta);
//                 lua->PushMetaTable(connection_meta);
//                 lua->SetMetaTable(-2);

//                 queries[event.conn] = {};
//             } else {
//                 lua->PushBool(false);
//                 lua->PushString(PQerrorMessage(event.conn));
//             }

//             if (lua->PCall(2, 0, -4) != 0) {
//                 lua->Pop();
//             }
//             lua->Pop();
//             lua->ReferenceFree(event.callback);

//             it = connections.erase(it);
//         } else {
//             ++it;
//         }
//     }
// }

// inline void query_failed(GLua::ILuaInterface* lua, PGconn* conn,
//                          QueryEvent& event) {
//     lua->GetField(GLua::INDEX_GLOBAL, "ErrorNoHaltWithStack");
//     lua->ReferencePush(event.callback);
//     lua->PushBool(false);
//     lua->PushString(PQerrorMessage(conn));
//     if (lua->PCall(2, 0, -4) != 0) {
//         lua->Pop();
//     }
//     lua->Pop();
//     lua->ReferenceFree(event.callback);
// }

// inline void parse_query_result(GLua::ILuaInterface* lua,
//                                const PGresult* result) {
//     lua->CreateTable();
//     for (int i = 0; i < PQntuples(result); i++) {
//         lua->PushNumber(i);
//         lua->CreateTable();
//         for (int j = 0; j < PQnfields(result); j++) {
//             lua->PushString(PQfname(result, j));
//             lua->PushString(PQgetvalue(result, i, j));
//             lua->SetTable(-3);
//         }
//         lua->SetTable(-3);
//     }
// }

// inline void process_query_result(GLua::ILuaInterface* lua, QueryEvent& event,
//                                  const PGresult* result) {
//     auto status = PQresultStatus(result);

//     lua->GetField(GLua::INDEX_GLOBAL, "ErrorNoHaltWithStack");
//     lua->ReferencePush(event.callback);

//     if (status == PGRES_BAD_RESPONSE || status == PGRES_NONFATAL_ERROR ||
//         status == PGRES_FATAL_ERROR) {
//         lua->PushBool(false);
//         lua->PushString(PQresultErrorMessage(result));
//     } else {
//         lua->PushBool(true);
//         parse_query_result(lua, result);
//     }

//     if (lua->PCall(2, 0, -4) != 0) {
//         lua->Pop();
//     }
//     lua->Pop();
// }

// inline void process_queries(GLua::ILuaInterface* lua) {
//     for (auto& [conn, queue] : queries) {
//         while (!queue.empty()) {
//             auto& event = queue.front();
//             if (!event.sent) {
//                 if (PQsendQuery(conn, event.query.c_str()) == 0) {
//                     query_failed(lua, conn, event);
//                     queue.pop();
//                     continue;
//                 }
//                 event.sent = true;
//             }

//             if (!event.flushed) {
//                 auto status = PQflush(conn);
//                 if (status == -1) {
//                     query_failed(lua, conn, event);
//                     queue.pop();
//                     continue;
//                 }
//                 event.flushed = status == 0;
//             }

//             PQconsumeInput(conn);

//             if (PQisBusy(conn) == 1) {
//                 break;
//             }

//             if (auto result = PQgetResult(conn)) {
//                 process_query_result(lua, event, result);
//                 PQclear(result);
//             } else {
//                 lua->ReferenceFree(event.callback);
//                 queue.pop();
//             }
//         }
//     }
// }

lua_protected_fn(loop) {
    pgsqloo::process_pending_connections(lua);

    for (auto state : pgsqloo::connections) {
        pgsqloo::process_notifications(lua, state);
        pgsqloo::process_queries(lua, state);
    }

    return 0;
}

lua_protected_fn(connect) {
    lua->CheckType(1, GLua::Type::String);
    lua->CheckType(2, GLua::Type::Function);

    auto url = lua->GetString(1);
    GLua::AutoReference callback(lua, 2);

    pgsqloo::connect(lua, url, std::move(callback));

    return 0;
}

lua_protected_fn(query) {
    lua->CheckType(1, pgsqloo::connection_meta);
    lua->CheckType(2, GLua::Type::String);
    lua->CheckType(3, GLua::Type::Function);

    auto state = lua_connection_state();

    pgsqloo::SimpleCommand command = {lua->GetString(2)};
    pgsqloo::Query query = {std::move(command)};
    if (!lua->IsType(3, GLua::Type::Nil)) {
        query.callback = GLua::AutoReference(lua, 3);
    }

    state->queries.push(std::move(query));
    return 0;
}

// int query(lua_State* L) {
//     auto lua = reinterpret_cast<GLua::ILuaInterface*>(L->luabase);
//     lua->SetState(L);

//     lua->CheckType(1, connection_meta);
//     lua->CheckType(2, GLua::Type::String);
//     lua->CheckType(3, GLua::Type::Function);

//     auto conn = lua->GetUserType<PGconn>(1, connection_meta);
//     auto query = lua->GetString(2);

//     lua->Push(3);
//     int callback = lua->ReferenceCreate();

//     auto& queue = queries[conn];
//     queue.push({conn, callback, query});

//     return 0;
// }

inline void register_connection_mt(GLua::ILuaInterface* lua) {
    pgsqloo::connection_meta = lua->CreateMetaTable("PGconn");

    lua->PushCFunction(index_connection);
    lua->SetField(-2, "__index");

    lua->PushCFunction(newindex_connection);
    lua->SetField(-2, "__newindex");

    lua->PushCFunction(gc_connection);
    lua->SetField(-2, "__gc");

    lua->PushCFunction(query);
    lua->SetField(-2, "query");

    lua->Pop();
}

inline void make_global_table(GLua::ILuaInterface* lua) {
    lua->CreateTable();

    lua->PushCFunction(connect);
    lua->SetField(-2, "connect");

    lua->SetField(GLua::INDEX_GLOBAL, "pg");
}

inline void register_loop_hook(GLua::ILuaInterface* lua) {
    lua->GetField(GLua::INDEX_GLOBAL, "hook");
    lua->GetField(-1, "Add");
    lua->PushString("Think");
    lua->PushString("pg_loop");
    lua->PushCFunction(loop);
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
