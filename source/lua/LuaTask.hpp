#pragma once

#include "lua/LuaBaseObject.hpp"
#include "ITask.hpp"
#include <string_view>
#include <memory>

namespace pgsqloo {
    struct LuaTask : public LuaBaseObject<LuaTask> {
        std::shared_ptr<IResultTask> m_Task;

        LuaTask(std::shared_ptr<IResultTask> task) : m_Task(task) {}

        static int MetaType;
        static void Initialize(GarrysMod::Lua::ILuaInterface* LUA);
    };
}