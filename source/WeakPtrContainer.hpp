#pragma once

#include <vector>
#include <memory>

namespace pgsqloo {
    template<typename T>
    class WeakPtrContainer {
    public:
        typedef std::weak_ptr<T> PointerType;
        typedef std::vector<PointerType> ContainerType;

    private:
        ContainerType m_Pointers;
        int m_Threshold = 10;
        int m_ThresholdMultiplier = 10;

    public:
        WeakPtrContainer() = default;
        WeakPtrContainer(WeakPtrContainer&&) = default;
        WeakPtrContainer& operator=(WeakPtrContainer&&) = default;

        inline void Add(PointerType ptr) {
            m_Pointers.push_back(ptr);
        }

        inline void Remove(PointerType ptr) {
            auto it = std::find(m_Pointers.begin(), m_Pointers.end(), ptr);
            if (it != m_Pointers.end()) { m_Pointers.erase(it); }
        }

        inline void ForceCleanup() {
            for (auto it = m_Pointers.begin(); it != m_Pointers.end();) {
                if (it->expired()) {
                    it = m_Pointers.erase(it);
                } else {
                    ++it;
                }
            }
            m_Threshold = (int)m_Pointers.size() + m_ThresholdMultiplier;
        }

        inline void Cleanup() {
            if (m_Pointers.size() > m_Threshold) ForceCleanup();
        }

        ContainerType::iterator begin() { return m_Pointers.begin(); }
        ContainerType::iterator end() { return m_Pointers.end(); }
    };
}