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
    if (!push_on_notify(lua, state)) {
        return;
    }

    if (state->queries.empty() && PQconsumeInput(state->conn.get()) == 0) {
        // we consumed input
        // but there was some error
        // TODO: update connection state
        return;
    }

    PGnotify* notify;
    while ((notify = PQnotifies(state->conn.get()))) {
        lua->Push(-1);  // copy on_notify

        lua->PushString(notify->relname);  // arg 1 channel name
        lua->PushString(notify->extra);    // arg 2 payload
        lua->PushNumber(notify->be_pid);   // arg 3 backend pid

        pcall(lua, 3, 0);

        PQfreemem(notify);
    }

    lua->Pop();  // pop on_notify
}
