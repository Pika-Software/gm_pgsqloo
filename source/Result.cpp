#include "Result.hpp"

#include <pqxx/result>

#include <fmt/format.h>
#include <Core.hpp>

using namespace pgsqloo;

void Result::ProcessField(GarrysMod::Lua::ILuaBase* LUA,
                          const pqxx::field& field) {
    if (field.is_null()) return LUA->PushNil();

    char category = m_Types ? m_Types->GetCategory(field.type()) : 'S';
    switch (category) {
        case 'B':
            LUA->PushBool(field.as<bool>());
            break;
        case 'N':
            LUA->PushNumber(field.as<double>());
            break;
        default:
            auto str = field.view();
            LUA->PushString(str.data(), (unsigned int)str.size());
            break;
    }
}
void Result::ProcessRow(GarrysMod::Lua::ILuaBase* LUA, const pqxx::row& row) {
    LUA->CreateTable();
    for (const auto& field : row) {
        const char* name = field.name();
        if (isNumeric || strcmp(name, "?column?") == 0)
            LUA->PushNumber(field.num() + 1);
        else
            LUA->PushString(name);
        ProcessField(LUA, field);
        LUA->SetTable(-3);
    }
}
void Result::ProcessResult(GarrysMod::Lua::ILuaBase* LUA) {
    if (std::holds_alternative<pqxx::row>(m_Result)) { // Single row
         ProcessRow(LUA, std::get<pqxx::row>(m_Result));
         return;
    } else if (std::holds_alternative<pqxx::result>(m_Result)) { // Multiple rows
        const auto& result = std::get<pqxx::result>(m_Result);
        LUA->CreateTable();
        for (pqxx::result::size_type i = 0; i < std::size(result); ++i) {
            LUA->PushNumber(i + 1);
            ProcessRow(LUA, result[i]);
            LUA->SetTable(-3);
        }
    } else { // Invalid result
        LUA->PushNil();
    }
}

void Result::Push(GarrysMod::Lua::ILuaBase* LUA) {
    if (m_DataRef.Push()) return;

    ProcessResult(LUA);
    if (LUA->IsType(-1, GarrysMod::Lua::Type::Table)) {
        m_DataRef.Setup(LUA);
        m_DataRef.Create(-1);
    }
}

pqxx::params pgsqloo::MakeParams(GarrysMod::Lua::ILuaBase* LUA, int index) {
    if (!LUA->IsType(index, GarrysMod::Lua::Type::Table)) return {};
    
    pqxx::params params;
    int len = LUA->ObjLen(index);
    for (int i = 0; i < len; i++) {
        LUA->PushNumber(i + 1);
        LUA->GetTable(index > 0 ? index : index - 1);
        switch (LUA->GetType(-1)) {
        case GarrysMod::Lua::Type::Bool:
            params.append(LUA->GetBool(-1));
            break;
        case GarrysMod::Lua::Type::Number:
            params.append(LUA->GetNumber(-1));
            break;
        case GarrysMod::Lua::Type::String:
            params.append(utils::GetString(LUA));
            break;
        default:
            throw std::runtime_error(fmt::format("unknown type {} at index {} of params table", LUA->GetTypeName(LUA->GetType(-1)), i));
        }
        LUA->Pop();
    }
    return params;
}
