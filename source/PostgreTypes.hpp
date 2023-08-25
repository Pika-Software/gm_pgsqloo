#pragma once

#include <unordered_map>

namespace pqxx {
    using oid = unsigned int;
}

namespace pgsqloo {
    struct PostgreTypes {
        std::unordered_map<pqxx::oid, char> m_Types;

        inline void SetType(pqxx::oid id, char category) {
            m_Types.insert_or_assign(id, category);
        }

        inline char GetCategory(pqxx::oid id) {
            auto result = m_Types.find(id);
            return result != m_Types.end() ? result->second : 'X';
        }
    };
}  // namespace pgsqloo
