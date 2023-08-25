#include "Core.hpp"

#include "lua/LuaConnection.hpp"
#include "lua/LuaTask.hpp"
#include "utils.hpp"
#include "Worker.hpp"

using namespace pgsqloo;

namespace LuaFuncs {
    SAFE_LUA_FUNCTION(Think) {
        g_Core->Think(LUA);
        return 0;
    }
}

Core::Core(GarrysMod::Lua::ILuaInterface* LUA) {
    this->LUA = LUA;

    LUA->CreateTable();
        LuaConnection::Initialize(LUA);
        LuaTask::Initialize(LUA);
    LUA->SetField(GarrysMod::Lua::INDEX_GLOBAL, "pgsqloo");

    LUA->GetField(GarrysMod::Lua::INDEX_GLOBAL, "timer");
        LUA->GetField(-1, "Create");
            LUA->PushString("__pgsqloo_think");
            LUA->PushNumber(0);
            LUA->PushNumber(0);
            LUA->PushCFunction(LuaFuncs::Think);
        LUA->Call(4, 0);
    LUA->Pop();
}

void Core::Close() {
    Think(LUA);

    if (!m_Factory->IsEmpty())
        LUA->Msg("[pgsqloo]: %d objects were not cleaned up!\n", m_Factory->GetAllocated());

    if (m_Factory.use_count() != 1)
        LUA->Msg("[pgsqloo]: %d factory references were not cleaned up!\n", m_Factory.use_count() - 1);
}

void Core::Think(GarrysMod::Lua::ILuaInterface* LUA) {
    for (const auto& ptr : m_Workers) {
        if (auto worker = ptr.lock())
            worker->Think(LUA);
    }
    m_Workers.Cleanup();
    m_Factory->Cleanup();
}

std::shared_ptr<Worker> Core::CreateWorker() {
    auto worker = m_Factory->Make<Worker>();
    m_Workers.Add(worker);
    return worker;
}

void Core::DebugMsg(const std::string& msg) {
    LUA->MsgColour(Color(204, 153, 255), msg.c_str());
}
