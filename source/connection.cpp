#include "pgsqloo.hpp"

using namespace pgsqloo;

std::vector<Connection*> pgsqloo::connections = {};

struct ConnectionEvent {
    PGconnPtr conn;
    GLua::AutoReference callback;
    PostgresPollingStatusType status = PGRES_POLLING_WRITING;
    bool is_reset = false;
};

std::vector<ConnectionEvent> pending_connections = {};

inline bool socket_is_ready(PGconn* conn, PostgresPollingStatusType status) {
    if (status == PGRES_POLLING_READING || status == PGRES_POLLING_WRITING) {
        auto socket = check_socket_status(conn);
        return socket.failed ||
               ((status == PGRES_POLLING_READING && socket.read_ready) ||
                (status == PGRES_POLLING_WRITING && socket.write_ready));
    }
    return true;
}

void pgsqloo::connect(GLua::ILuaInterface* lua, std::string_view url,
                      GLua::AutoReference&& callback) {
    auto conn = PGconnPtr(PQconnectStart(url.data()), &PQfinish);
    auto conn_ptr = conn.get();

    if (!conn) {
        // funnily enough, this probably will instead throw a std::bad_alloc
        throw std::runtime_error("failed to allocate connection");
    }

    if (PQstatus(conn_ptr) == CONNECTION_BAD ||
        PQsetnonblocking(conn_ptr, 1) != 0) {
        throw std::runtime_error(PQerrorMessage(conn_ptr));
    }

    auto event = ConnectionEvent{std::move(conn), std::move(callback)};
    pending_connections.push_back(std::move(event));
}

// returns true if we finished polling
// returns false if we need to poll again
inline bool poll_pending_connection(GLua::ILuaInterface* lua,
                                    ConnectionEvent& event) {
    if (!socket_is_ready(event.conn.get(), event.status)) {
        lua->Msg("socket is not ready (%s)\n",
                 event.status == PGRES_POLLING_READING ? "reading" : "writing");
        return false;
    }

    // TODO: handle reset

    event.status = PQconnectPoll(event.conn.get());
    lua->Msg("status: %d (%d)\n", event.status, PQstatus(event.conn.get()));
    if (event.status == PGRES_POLLING_OK) {
        auto state = new Connection{std::move(event.conn)};

        lua->CreateTable();
        state->lua_table = GLua::AutoReference(lua);

        connections.push_back(state);

        event.callback.Push();
        lua->PushBool(true);
        lua->PushUserType(state, connection_meta);
        lua->PushMetaTable(connection_meta);
        lua->SetMetaTable(-2);
        pcall(lua, 2, 0);

        return true;
    } else if (event.status == PGRES_POLLING_FAILED) {
        event.callback.Push();
        lua->PushBool(false);
        lua->PushString(PQerrorMessage(event.conn.get()));
        pcall(lua, 2, 0);

        return true;
    }

    return false;
}

void pgsqloo::process_pending_connections(GLua::ILuaInterface* lua) {
    for (auto it = pending_connections.begin();
         it != pending_connections.end();) {
        auto& event = *it;

        if (poll_pending_connection(lua, event)) {
            it = pending_connections.erase(it);
        } else {
            ++it;
        }
    }
}
