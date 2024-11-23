#pragma once

#include <GarrysMod/Lua/AutoReference.h>
#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaInterface.h>
#include <libpq-fe.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <queue>
#include <string_view>
#include <variant>
#include <vector>

namespace GLua = GarrysMod::Lua;

#define lua_interface()                                 \
    reinterpret_cast<GLua::ILuaInterface*>(L->luabase); \
    L->luabase->SetState(L)

#define lua_protected_fn(name)                                                 \
    int name##__Imp(GLua::ILuaInterface* lua);                                 \
    int name(lua_State* L) {                                                   \
        auto lua = lua_interface();                                            \
        const auto start = std::chrono::high_resolution_clock::now();          \
        try {                                                                  \
            int ret = name##__Imp(lua);                                        \
            const auto end = std::chrono::high_resolution_clock::now();        \
            const std::chrono::duration<double, std::milli> dur = end - start; \
            if (dur.count() > 1)                                               \
                lua->Msg(#name " execution time: %.2fms\n", dur.count());      \
            return ret;                                                        \
        } catch (const std::exception& e) {                                    \
            lua->ThrowError(e.what());                                         \
            return 0;                                                          \
        }                                                                      \
    }                                                                          \
    int name##__Imp([[maybe_unused]] GarrysMod::Lua::ILuaInterface* lua)

namespace async_postgres {
    struct SocketStatus {
        bool read_ready = false;
        bool write_ready = false;
        bool failed = false;
    };

    struct SimpleCommand {
        std::string command;
    };

    struct ParameterizedCommand {
        std::string command;
        std::vector<std::string> values;
    };

    struct Query {
        std::variant<SimpleCommand, ParameterizedCommand> command;
        GLua::AutoReference callback;
        bool sent = false;
        bool flushed = false;
    };

    struct ResetEvent {
        std::vector<GLua::AutoReference> callbacks;
        PostgresPollingStatusType status = PGRES_POLLING_WRITING;
    };

    using PGconnPtr = std::unique_ptr<PGconn, decltype(&PQfinish)>;

    struct Connection {
        PGconnPtr conn;
        GLua::AutoReference lua_table;
        std::queue<Query> queries;
        std::optional<ResetEvent> reset_event;
        bool receive_notifications =
            false;  // enabled if on_notify lua field is set

        Connection(GLua::ILuaInterface* lua, PGconnPtr&& conn);
        ~Connection();
    };

    extern int connection_meta;
    extern std::vector<Connection*> connections;

    // connection.cpp
    void connect(GLua::ILuaInterface* lua, std::string_view url,
                 GLua::AutoReference&& callback);
    void process_pending_connections(GLua::ILuaInterface* lua);

    void reset(GLua::ILuaInterface* lua, Connection* state,
               GLua::AutoReference&& callback);
    void process_reset(GLua::ILuaInterface* lua, Connection* state);

    // notifications.cpp
    void process_notifications(GLua::ILuaInterface* lua, Connection* state);

    // query.cpp
    void process_queries(GLua::ILuaInterface* lua, Connection* state);

    // result.cpp
    void create_result_table(GLua::ILuaInterface* lua, PGresult* result);

    // misc.cpp
    void register_misc_connection_functions(GLua::ILuaInterface* lua);

    // util.cpp
    std::string_view get_string(GLua::ILuaInterface* lua, int index = -1);
    void pcall(GLua::ILuaInterface* lua, int nargs, int nresults);
    SocketStatus check_socket_status(PGconn* conn);
};  // namespace async_postgres
