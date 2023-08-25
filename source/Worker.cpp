#include "Worker.hpp"

#include "Core.hpp"
#include <GarrysMod/Lua/Interface.h>
#include <algorithm>

using namespace pgsqloo;

template <class Value, class Mutex>
Value PopTask(std::deque<Value>& queue, Mutex& mutex) {
    std::lock_guard<Mutex> guard(mutex);
    Value value = queue.front();
    queue.pop_front();
    return value;
}

void Worker::WorkerFunc(std::shared_ptr<WorkerData> data) {
    while (data && data->IsRunning()) {
        data->m_Notifier.WaitForNotify();

        while (!data->m_WorkerQueue.empty()) {
            try {
                auto task = PopTask(data->m_WorkerQueue, data->m_WorkerQueueMutex);
                task->Process();
            } catch (const std::exception &ex) {
                if (g_Core)
                    g_Core->LUA->ErrorNoHalt(
                        "error happened in pgsqloo worker: %s\n", ex.what());
            }
        }
    }
}

void Worker::Think(GarrysMod::Lua::ILuaInterface *LUA) {
    while (m_Data && !m_Data->m_ThinkQueue.empty()) {
        try {
            auto task = PopTask(m_Data->m_ThinkQueue, m_Data->m_ThinkQueueMutex);
            task->Finalize(LUA);
        } catch (const std::exception &ex) {
            if (g_Core)
                g_Core->LUA->ErrorNoHalt(
                    "error happened in pgsqloo think: %s\n", ex.what());
        }
    }
}
void Worker::Start() {
    Stop();
    m_Data = std::make_shared<WorkerData>();
    m_Data->m_Stopped = false;
    m_Thread = std::thread(&Worker::WorkerFunc, m_Data);
}
void Worker::Stop() {
    if (IsRunning()) {
        m_Data->m_Stopped = true;
        m_Data->m_Notifier.Notify();
        if (m_Thread.joinable()) m_Thread.join();
        if (g_Core) Think(g_Core->LUA);
    }
}
void Worker::Abort() {
    if (IsRunning()) {
        m_Data->m_Stopped = true;
        {
            std::lock_guard<std::mutex> guard(m_Data->m_WorkerQueueMutex);
            m_Data->m_WorkerQueue.clear();
        }
        {
            std::lock_guard<std::mutex> guard(m_Data->m_ThinkQueueMutex);
            m_Data->m_ThinkQueue.clear();
        }
        m_Data->m_Notifier.Notify();
        if (m_Thread.joinable()) m_Thread.detach();
    }
}
