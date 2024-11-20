#include <Platform.hpp>

#include "async_postgres.hpp"

using namespace async_postgres;

std::string_view async_postgres::get_string(GLua::ILuaInterface* lua,
                                            int index) {
    unsigned int len = 0;
    const char* str = lua->GetString(index, &len);
    return {str, len};
}

void print_stack(GLua::ILuaInterface* lua) {
    lua->Msg("stack: %d\n", lua->Top());
    for (int i = 1; i <= lua->Top(); i++) {
        lua->GetField(GLua::INDEX_GLOBAL, "tostring");
        lua->Push(i);
        lua->Call(1, 1);
        lua->Msg("\t%d: [%s] %s\n", i, lua->GetTypeName(lua->GetType(i)),
                 lua->GetString(-1));
        lua->Pop();
    }
}

void async_postgres::pcall(GLua::ILuaInterface* lua, int nargs, int nresults) {
    lua->GetField(GLua::INDEX_GLOBAL, "ErrorNoHaltWithStack");
    lua->Insert(-nargs - 2);
    if (lua->PCall(nargs, nresults, -nargs - 2) != 0) {
        lua->Pop();
    }

    lua->Pop();  // ErrorNoHaltWithStack
}

#if SYSTEM_IS_WINDOWS
#include <WinSock2.h>
#define poll WSAPoll
#else
#include <poll.h>
#endif

SocketStatus async_postgres::check_socket_status(PGconn* conn) {
    SocketStatus status = {};
    int fd = PQsocket(conn);
    if (fd < 0) {
        status.failed = true;
        return status;
    }

    pollfd fds[1] = {{fd, POLLIN | POLLOUT, 0}};
    if (poll(fds, 1, 0) < 0) {
        status.failed = true;
    } else {
        status.read_ready = fds[0].revents & POLLIN;
        status.write_ready = fds[0].revents & POLLOUT;
        status.failed = fds[0].revents & POLLERR || fds[0].revents & POLLHUP ||
                        fds[0].revents & POLLNVAL;
    }

    return status;
}
