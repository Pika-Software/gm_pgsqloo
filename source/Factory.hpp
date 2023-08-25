#pragma once

#include "utils.hpp"

#include <memory>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include "Core.hpp"

namespace pgsqloo {
    class Factory : public std::enable_shared_from_this<Factory> {
        std::deque<std::function<void()>> m_Deleters;
        std::mutex m_Lock;
        bool changed = false;

        std::thread::id m_MainThread = std::this_thread::get_id();
        std::atomic<int> m_Allocated = 0;

        Factory() = default;

    public:
        inline bool IsInMainThread() {
            return m_MainThread == std::this_thread::get_id();
        }

        template<typename T>
        inline void Remove(T* ptr) {
            m_Allocated--;
            delete ptr;
        }

        ~Factory() { if (IsInMainThread()) Cleanup(); }

        static std::shared_ptr<Factory> CreateFactory() {
            return std::shared_ptr<Factory>(new Factory());
        }

        template<typename T, typename... Args>
        std::shared_ptr<T> Make(Args&&... args) {
            auto ptr = new T(std::forward<Args>(args)...);
            m_Allocated++;
            return std::shared_ptr<T>(ptr, [ref = weak_from_this()](T* ptr) {
                if (auto factory = ref.lock()) {
                    if (!factory->IsInMainThread()) {
                        std::lock_guard<std::mutex> lock(factory->m_Lock);
                        factory->changed = true;
                        factory->m_Deleters.emplace_back([factory, ptr]() {
                            factory->Remove(ptr);
                        });
                    } else {
                        // We can delete things in the main thread immediatly
                        factory->Remove(ptr);
                    }
                } else {
                    // probably some memory leak happened here
                    delete ptr;
                }
            });
        }

        inline void Cleanup() {
            if (!changed) return;
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                changed = false;
            }

            defer { m_Deleters.clear(); };
            for (const auto& deleter : m_Deleters) {
                deleter();
            }
        }

        inline bool IsEmpty() { return m_Allocated == 0; }
        inline int GetAllocated() { return m_Allocated; }
    };
}