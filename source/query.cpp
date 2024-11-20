#include "pgsqloo.hpp"

using namespace pgsqloo;

// returns true if query was sent
// returns false on error
inline bool send_query(PGconn* conn, Query& query) {
    if (const auto* command = std::get_if<SimpleCommand>(&query.command)) {
        return PQsendQuery(conn, command->command.c_str()) == 1;
    }
    return false;
}

void query_failed(GLua::ILuaInterface* lua, PGconn* conn, Query& query) {
    if (query.callback) {
        query.callback.Push();
        lua->PushBool(false);
        lua->PushString(PQerrorMessage(conn));
        pcall(lua, 2, 0);
    }
}

// returns true if poll was successful
// returns false if there was an error
inline bool poll_query(PGconn* conn, Query& query) {
    auto socket = check_socket_status(conn);
    if (socket.read_ready || socket.write_ready) {
        if (socket.read_ready && PQconsumeInput(conn) == 0) {
            return false;
        }

        if (!query.flushed) {
            query.flushed = PQflush(conn) == 0;
        }
    }
    return true;
}

bool bad_result(PGresult* result) {
    if (!result) {
        return true;
    }

    auto status = PQresultStatus(result);

    return status == PGRES_BAD_RESPONSE || status == PGRES_NONFATAL_ERROR ||
           status == PGRES_FATAL_ERROR;
}

void pgsqloo::process_queries(GLua::ILuaInterface* lua, Connection* state) {
    if (state->queries.empty()) {
        // no queries to process
        return;
    }

    auto& query = state->queries.front();
    if (!query.sent) {
        if (!send_query(state->conn.get(), query)) {
            query_failed(lua, state->conn.get(), query);
            state->queries.pop();
            return process_queries(lua, state);
        }

        query.sent = true;
        query.flushed = PQflush(state->conn.get()) == 0;
    }

    if (!poll_query(state->conn.get(), query)) {
        query_failed(lua, state->conn.get(), query);
        state->queries.pop();
        return process_queries(lua, state);
    }

    while (PQisBusy(state->conn.get()) == 0) {
        auto* result = PQgetResult(state->conn.get());
        if (!result) {
            // query is done
            state->queries.pop();
            return process_queries(lua, state);
        }

        if (query.callback) {
            query.callback.Push();
            if (!bad_result(result)) {
                lua->PushBool(true);
                create_result_table(lua, result);
            } else {
                lua->PushBool(false);
                lua->PushString(PQresultErrorMessage(result));
            }

            pcall(lua, 2, 0);
        }
        PQclear(result);
    }
}
