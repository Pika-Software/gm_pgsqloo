#pragma once

#include "WeakPtrContainer.hpp"
#include "Worker.hpp"
#include "Factory.hpp"
#include <GarrysMod/Lua/LuaInterface.h>
#include <memory>

namespace pgsqloo {
    class Core {
    public:
        WeakPtrContainer<Worker> m_Workers;
        GarrysMod::Lua::ILuaInterface* LUA = nullptr;
        std::shared_ptr<Factory> m_Factory = Factory::CreateFactory();

        Core(GarrysMod::Lua::ILuaInterface* LUA);

        void Close();
        void Think(GarrysMod::Lua::ILuaInterface* LUA);

        std::shared_ptr<Worker> CreateWorker();
        void DebugMsg(const std::string& msg);
    };

    extern std::unique_ptr<Core> g_Core;
}
