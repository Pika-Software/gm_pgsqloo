#pragma once

#include <GarrysMod/Lua/AutoReference.h>
#include <GarrysMod/Lua/LuaBase.h>

#include <variant>
#include <pqxx/result>
#include <pqxx/field>
#include <pqxx/params>

#include "PostgreTypes.hpp"

namespace pgsqloo {
    class Result {
        std::variant<std::monostate, pqxx::result, pqxx::row> m_Result;
        std::shared_ptr<PostgreTypes> m_Types;
        bool isNumeric = false;
        GarrysMod::Lua::AutoReference m_DataRef;

        void ProcessField(GarrysMod::Lua::ILuaBase* LUA,
                          const pqxx::field& field);
        void ProcessRow(GarrysMod::Lua::ILuaBase* LUA, const pqxx::row& row);
        void ProcessResult(GarrysMod::Lua::ILuaBase* LUA);

    public:
        Result() {}
        Result(pqxx::result result, std::shared_ptr<PostgreTypes> types = {})
            : m_Result(result), m_Types(types) {}
        Result(pqxx::row row, std::shared_ptr<PostgreTypes> types = {})
            : m_Result(row), m_Types(types) {}

        Result(Result&&) = default;
        Result& operator=(Result&&) = default;

        void SetNumeric(bool numeric) {
            isNumeric = numeric;
            m_DataRef.Free();
        }
        void Reset() {
            m_Result = std::monostate();
            m_Types.reset();
            m_DataRef.Free();
        }

        inline bool IsEmpty() const { return std::holds_alternative<std::monostate>(m_Result); }
        inline operator bool() const { return !IsEmpty(); }

        void Push(GarrysMod::Lua::ILuaBase* LUA);
    };

    pqxx::params MakeParams(GarrysMod::Lua::ILuaBase* LUA, int index);
}  // namespace pgsqloo
