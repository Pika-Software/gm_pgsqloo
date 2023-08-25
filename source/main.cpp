#include <GarrysMod/Lua/Interface.h>

#include <Platform.hpp>

#include "Core.hpp"

#if SYSTEM_IS_POSIX
#include <signal.h>
#endif

using namespace pgsqloo;

std::unique_ptr<Core> pgsqloo::g_Core;

GMOD_MODULE_OPEN() {
    auto ILUA = reinterpret_cast<GarrysMod::Lua::ILuaInterface*>(LUA);
    if (!ILUA->IsServer())
        LUA->ThrowError("pgsqloo only can run in serverside");

#if SYSTEM_IS_POSIX
    // Ignore connection breaks with database
    signal(SIGPIPE, SIG_IGN);
#endif

    g_Core = std::make_unique<Core>(ILUA);

    return 0;
}

GMOD_MODULE_CLOSE() {
    g_Core->Close();
    g_Core.reset();

    return 0;
}
