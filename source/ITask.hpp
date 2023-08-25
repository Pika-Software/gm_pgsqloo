#pragma once

#include <GarrysMod/Lua/LuaInterface.h>
#include <memory>
#include <variant>
#include "WorkerData.hpp"
#include "Result.hpp"
#include "utils.hpp"

namespace pgsqloo {
    struct WaitableTask : ITask {
        Notifier m_Notifier;

        bool HasFinished() const { return m_Notifier.finished; }
        void Finish(bool finalize = true) {
            if (auto queue = m_Queue.lock(); finalize && queue) queue->AddThinkTask(shared_from_this());
            m_Notifier.Finish();
        }
        void Wait() { m_Notifier.WaitForFinish(false); }
    };

    struct IResultTask : WaitableTask {
        virtual bool IsSuccess() const = 0;
        virtual bool IsError() const { return HasFinished() && !IsSuccess(); };
        virtual int PushResult(GarrysMod::Lua::ILuaInterface* LUA) = 0;
        virtual int PushError(GarrysMod::Lua::ILuaInterface* LUA) = 0;
    };

    template<class ResultType>
    struct ResultTask : IResultTask {
        std::variant<std::string, ResultType, std::monostate> m_Result = "unknown error";

        void SetResult(ResultType&& result) { m_Result = std::move(result); }
        void SetResult() { m_Result = std::monostate{}; }
        bool HasResult() const { return std::holds_alternative<ResultType>(m_Result); }
        ResultType& GetResult() { return std::get<ResultType>(m_Result); }
        ResultType&& ReleaseResult() { return std::get<ResultType>(std::move(m_Result)); }

        void SetError(std::string&& error) { m_Result = std::move(error); }
        const std::string& GetError() const { return std::get<std::string>(m_Result); }

        virtual bool IsSuccess() const { return !std::holds_alternative<std::string>(m_Result); }
        virtual int PushError(GarrysMod::Lua::ILuaInterface* LUA) {
            if (IsError())  utils::PushString(LUA, GetError());
            else            LUA->PushNil();
            return 1;
        }
    };

    struct BasicQueryTask : ResultTask<Result> {
        virtual int PushResult(GarrysMod::Lua::ILuaInterface* LUA) {
            if (IsSuccess() && HasResult()) GetResult().Push(LUA);
            else            LUA->PushNil();
            return 1;
        }
    };
}
