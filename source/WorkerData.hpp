#pragma once

#include "Notifier.hpp"

#include <memory>
#include <mutex>
#include <deque>

namespace pgsqloo {
    enum AddTaskTo_t {
        ADD_TASK_TO_HEAD,
        ADD_TASK_TO_TAIL,
    };

    struct WorkerData;
    struct ITask : std::enable_shared_from_this<ITask> {
        virtual ~ITask() = default;
        std::weak_ptr<WorkerData> m_Queue;

        virtual void Process() = 0;
        virtual void Finalize(GarrysMod::Lua::ILuaInterface* LUA) = 0;
    };

    class Worker;
    struct WorkerData : std::enable_shared_from_this<WorkerData> {
        Notifier m_Notifier;
        bool m_Stopped = true;

        std::mutex m_WorkerQueueMutex;
        std::deque<std::shared_ptr<ITask>> m_WorkerQueue;

        std::mutex m_ThinkQueueMutex;
        std::deque<std::shared_ptr<ITask>> m_ThinkQueue;

        void AddWorkerTask(std::shared_ptr<ITask> task, AddTaskTo_t to = ADD_TASK_TO_TAIL) {
            std::lock_guard<std::mutex> guard(m_WorkerQueueMutex);
            task->m_Queue = weak_from_this();
            if (to == ADD_TASK_TO_HEAD) m_WorkerQueue.push_front(std::move(task));
            else                        m_WorkerQueue.push_back(std::move(task));
            m_Notifier.Notify();
        }
        void AddThinkTask(std::shared_ptr<ITask> task, AddTaskTo_t to = ADD_TASK_TO_TAIL) {
            std::lock_guard<std::mutex> guard(m_ThinkQueueMutex);
            task->m_Queue = weak_from_this();
            if (to == ADD_TASK_TO_HEAD) m_ThinkQueue.push_front(std::move(task));
            else                        m_ThinkQueue.push_back(std::move(task));
        }
        inline bool IsRunning() const { return !m_Stopped; }
    };
}
