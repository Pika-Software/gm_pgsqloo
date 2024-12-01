#include "async_postgres.hpp"

using namespace async_postgres;

bool push_on_notify(GLua::ILuaInterface* lua, Connection* state) {
    state->lua_table.Push();
    lua->GetField(-1, "on_notify");
    lua->Remove(-2);  // remove the connection table

    if (lua->IsType(-1, GLua::Type::Nil)) {
        lua->Pop();
        return false;
    }
    return true;
}

void async_postgres::process_notifications(GLua::ILuaInterface* lua,
                                           Connection* state) {
    if (state->reset_event) {
        // don't process notifications while reconnecting
        return;
    }

    if (!state->receive_notifications) {
        return;
    }

    if (state->queries.empty() &&
        check_socket_status(state->conn.get()).read_ready &&
        PQconsumeInput(state->conn.get()) == 0) {
        // we consumed input
        // but there was some error
        return;
    }

    while (auto notify = pg::getNotify(state->conn)) {
        if (push_on_notify(lua, state)) {
            lua->PushString(notify->relname);  // arg 1 channel name
            lua->PushString(notify->extra);    // arg 2 payload
            lua->PushNumber(notify->be_pid);   // arg 3 backend pid

            pcall(lua, 3, 0);
        }
    }
}
