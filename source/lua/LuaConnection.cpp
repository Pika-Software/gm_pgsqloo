#include "lua/LuaConnection.hpp"

#include "utils.hpp"
#include "Core.hpp"
#include "Connection.hpp"
#include "lua/LuaTask.hpp"
#include <fmt/format.h>
#include <pqxx/pqxx>
#include "Result.hpp"

using namespace pgsqloo;

namespace LuaFuncs {
    SAFE_LUA_FUNCTION(pgsqloo_Connection) {
        std::string_view url = utils::CheckString(LUA, 1);
        LuaConnection::CreateAndPush(url);
        return 1;
    }

    SAFE_LUA_FUNCTION(__tostring) {
        auto conn = LuaConnection::CheckFromLua();
        std::string str = fmt::format("PostgreConnection ({})", conn->m_Inner ? conn->m_Inner->GetUrl() : "<null>");
        utils::PushString(LUA, str);
        return 1;
    }

    SAFE_LUA_FUNCTION(Connect) {
        auto conn = LuaConnection::CheckFromLua();
        LuaTask::CreateAndPush(conn->m_Inner->Connect({LUA, 1}));
        return 1;
    }

    SAFE_LUA_FUNCTION(Query) {
        auto conn = LuaConnection::CheckFromLua();
        auto sql = utils::CheckString(LUA, 2);

        GarrysMod::Lua::AutoReference callback;
        std::optional<pqxx::params> params;

        if (auto pos = utils::FindType(LUA, GarrysMod::Lua::Type::Table, { 3, 4 })) {
            params = MakeParams(LUA, *pos);
        }
        if (auto pos = utils::FindType(LUA, GarrysMod::Lua::Type::Function, { 3, 4 })) {
            callback = GarrysMod::Lua::AutoReference(LUA, *pos);
        }

        LuaTask::CreateAndPush(conn->m_Inner->Query(sql, std::move(callback), std::move(params)));
        return 1;
    }

    SAFE_LUA_FUNCTION(PreparedQuery) {
        auto conn = LuaConnection::CheckFromLua();
        auto name = utils::CheckString(LUA, 2);

        GarrysMod::Lua::AutoReference callback;
        pqxx::params params;

        if (auto pos = utils::FindType(LUA, GarrysMod::Lua::Type::Table, { 3, 4 })) {
            params = MakeParams(LUA, *pos);
        }
        if (auto pos = utils::FindType(LUA, GarrysMod::Lua::Type::Function, { 3, 4 })) {
            callback = GarrysMod::Lua::AutoReference(LUA, *pos);
        }

        LuaTask::CreateAndPush(conn->m_Inner->PreparedQuery(name, std::move(params), std::move(callback)));
        return 1;
    }

    SAFE_LUA_FUNCTION(Disconnect) {
        auto conn = LuaConnection::CheckFromLua();
        bool gracefully = LUA->IsType(2, GarrysMod::Lua::Type::Bool) ? LUA->GetBool(2) : false;
        conn->m_Inner->Disconnect(gracefully);
        return 0;
    }

    SAFE_LUA_FUNCTION(IsConnected) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushBool(conn->m_Inner->IsConnected());
        return 1;
    }

    SAFE_LUA_FUNCTION(IsConnecting) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushBool(conn->m_Inner->IsConnecting());
        return 1;
    }

    SAFE_LUA_FUNCTION(Database) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushString(conn->m_Inner->GetConnection()->dbname());
        return 1;
    }

    SAFE_LUA_FUNCTION(Username) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushString(conn->m_Inner->GetConnection()->username());
        return 1;
    }

    SAFE_LUA_FUNCTION(Hostname) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushString(conn->m_Inner->GetConnection()->hostname());
        return 1;
    }

    SAFE_LUA_FUNCTION(Port) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushString(conn->m_Inner->GetConnection()->port());
        return 1;
    }

    SAFE_LUA_FUNCTION(BackendPID) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushNumber(conn->m_Inner->GetConnection()->backendpid());
        return 1;
    }

    SAFE_LUA_FUNCTION(ProtocolVersion) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushNumber(conn->m_Inner->GetConnection()->protocol_version());
        return 1;
    }

    SAFE_LUA_FUNCTION(ServerVersion) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushNumber(conn->m_Inner->GetConnection()->server_version());
        return 1;
    }

    SAFE_LUA_FUNCTION(GetClientEncoding) {
        auto conn = LuaConnection::CheckFromLua();
        utils::PushString(LUA, conn->m_Inner->GetConnection()->get_client_encoding());
        return 1;
    }

    SAFE_LUA_FUNCTION(SetClientEncoding) {
        auto conn = LuaConnection::CheckFromLua();
        auto encoding = LUA->CheckString(2);
        conn->m_Inner->GetConnection()->set_client_encoding(encoding);
        return 0;
    }

    SAFE_LUA_FUNCTION(EncodingID) {
        auto conn = LuaConnection::CheckFromLua();
        LUA->PushNumber(conn->m_Inner->GetConnection()->encoding_id());
        return 1;
    }

    SAFE_LUA_FUNCTION(Prepare) {
        auto conn = LuaConnection::CheckFromLua();
        auto name = LUA->CheckString(2);
        auto sql = LUA->CheckString(3);
        conn->m_Inner->Prepare(name, sql);
        return 0;
    }

    SAFE_LUA_FUNCTION(Unprepare) {
        auto conn = LuaConnection::CheckFromLua();
        auto name = LUA->CheckString(2);
        conn->m_Inner->Unprepare(name);
        return 0;
    }
}

int LuaConnection::MetaType = 0;
LuaConnection::LuaConnection(std::string_view url) {
    m_Inner = g_Core->m_Factory->Make<Connection>(url);
}

LuaConnection::~LuaConnection() {}

void LuaConnection::Initialize(GarrysMod::Lua::ILuaInterface* LUA) {
    utils::SetFunction(LUA, LuaFuncs::pgsqloo_Connection, "Connection");

    MetaType = LUA->CreateMetaTable("PostgreConnection");
        utils::SetFunction(LUA, LuaConnection::HandleGC, "__gc");
        utils::SetFunction(LUA, LuaConnection::HandleIndex, "__index");
        utils::SetFunction(LUA, LuaConnection::HandleNewIndex, "__newindex");
        utils::SetFunction(LUA, LuaConnection::HandleGetTable, "getTable");

        utils::SetFunction(LUA, LuaFuncs::__tostring, "__tostring");
        utils::SetFunction(LUA, LuaFuncs::Connect, "Connect");
        utils::SetFunction(LUA, LuaFuncs::Query, "Query");
        utils::SetFunction(LUA, LuaFuncs::PreparedQuery, "PreparedQuery");
        utils::SetFunction(LUA, LuaFuncs::Disconnect, "Disconnect");
        utils::SetFunction(LUA, LuaFuncs::IsConnected, "IsConnected");
        utils::SetFunction(LUA, LuaFuncs::IsConnecting, "IsConnecting");
        utils::SetFunction(LUA, LuaFuncs::Database, "Database");
        utils::SetFunction(LUA, LuaFuncs::Username, "Username");
        utils::SetFunction(LUA, LuaFuncs::Hostname, "Hostname");
        utils::SetFunction(LUA, LuaFuncs::Port, "Port");
        utils::SetFunction(LUA, LuaFuncs::BackendPID, "BackendPID");
        utils::SetFunction(LUA, LuaFuncs::ProtocolVersion, "ProtocolVersion");
        utils::SetFunction(LUA, LuaFuncs::ServerVersion, "ServerVersion");
        utils::SetFunction(LUA, LuaFuncs::GetClientEncoding, "GetClientEncoding");
        utils::SetFunction(LUA, LuaFuncs::SetClientEncoding, "SetClientEncoding");
        utils::SetFunction(LUA, LuaFuncs::EncodingID, "EncodingID");
        utils::SetFunction(LUA, LuaFuncs::Prepare, "Prepare");
        utils::SetFunction(LUA, LuaFuncs::Unprepare, "Unprepare");

        utils::SetFunction(LUA, utils::EmptyFunc, "OnConnected");
    LUA->Pop();
}
