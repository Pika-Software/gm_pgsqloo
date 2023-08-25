#pragma once

#include <GarrysMod/Lua/LuaInterface.h>

#include <deque>
#include <thread>
#include <vector>

#include "WorkerData.hpp"

namespace pgsqloo {
    class Worker {
        std::thread m_Thread;
        std::shared_ptr<WorkerData> m_Data;

        static void WorkerFunc(std::shared_ptr<WorkerData> data);

    public:
        Worker() = default;
        Worker(Worker&&) = default;
        Worker& operator=(Worker&&) = default;
        ~Worker() { Stop(); }

        void Think(GarrysMod::Lua::ILuaInterface* LUA);

        void Start();
        void Stop();
        void Abort();
        void Restart(bool gracefully = true) {
            if (gracefully) Stop();
            else Abort();
            Start();
        }

        inline void AddWorkerTask(std::shared_ptr<ITask> task, AddTaskTo_t to = ADD_TASK_TO_TAIL) { if (m_Data) m_Data->AddWorkerTask(task, to); }
        inline void AddThinkTask(std::shared_ptr<ITask> task, AddTaskTo_t to = ADD_TASK_TO_TAIL) { if (m_Data) m_Data->AddThinkTask(task, to); }
        inline bool IsRunning() const { return m_Data && m_Data->IsRunning(); }
    };
}
