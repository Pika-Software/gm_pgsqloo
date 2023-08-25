#include "lua/LuaTask.hpp"

#include <fmt/format.h>

using namespace pgsqloo;

namespace LuaFuncs {
    SAFE_LUA_FUNCTION(Wait) {
        auto task = LuaTask::CheckFromLua();
        if (task->m_Task) {
            task->m_Task->Wait();
            g_Core->Think(LUA);
        }
        LUA->Push(1);
        return 1;
    }

    SAFE_LUA_FUNCTION(IsSuccess) {
        auto task = LuaTask::CheckFromLua();
        LUA->PushBool(task->m_Task ? task->m_Task->IsSuccess() : false);
        return 1;
    }

    SAFE_LUA_FUNCTION(IsError) {
        auto task = LuaTask::CheckFromLua();
        LUA->PushBool(task->m_Task ? task->m_Task->IsError() : false);
        return 1;
    }

    SAFE_LUA_FUNCTION(IsProcessing) {
        auto task = LuaTask::CheckFromLua();
        LUA->PushBool(task->m_Task ? !task->m_Task->HasFinished() : false);
        return 1;
    }

    SAFE_LUA_FUNCTION(GetResult) {
        auto task = LuaTask::CheckFromLua();
        if (task->m_Task) return task->m_Task->PushResult(LUA);
        return 0;
    }

    SAFE_LUA_FUNCTION(GetError) {
        auto task = LuaTask::CheckFromLua();
        if (task->m_Task) return task->m_Task->PushError(LUA);
        if (task->m_Task->IsError()) {
            LUA->PushString("unknown error");
            return 1;
        }
        return 0;
    }

    SAFE_LUA_FUNCTION(Task__tostring) {
        auto task = LuaTask::CheckFromLua();
        if (task->m_Task) {
            std::string str = fmt::format("PostgreTask ({})", task->m_Task->HasFinished() ? (task->m_Task->IsSuccess() ? "success" : "error") : "processing");
            utils::PushString(LUA, str);
        } else {
            LUA->PushString("PostgreTask (null)");
        }
        return 1;
    }
}

int LuaTask::MetaType = 0;
void LuaTask::Initialize(GarrysMod::Lua::ILuaInterface* LUA) {
    MetaType = LUA->CreateMetaTable("PostgreTask");
        utils::SetFunction(LUA, LuaTask::HandleGC, "__gc");
        utils::SetFunction(LUA, LuaTask::HandleIndex, "__index");
        utils::SetFunction(LUA, LuaTask::HandleNewIndex, "__newindex");
        utils::SetFunction(LUA, LuaTask::HandleGetTable, "getTable");
        utils::SetFunction(LUA, LuaFuncs::Task__tostring, "__tostring");

        utils::SetFunction(LUA, LuaFuncs::Wait, "Wait");
        utils::SetFunction(LUA, LuaFuncs::IsSuccess, "IsSuccess");
        utils::SetFunction(LUA, LuaFuncs::IsError, "IsError");
        utils::SetFunction(LUA, LuaFuncs::GetResult, "GetResult");
        utils::SetFunction(LUA, LuaFuncs::GetError, "GetError");
    LUA->Pop();
}
