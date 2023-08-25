#include "Connection.hpp"

#include "Core.hpp"
#include "utils.hpp"
#include "Result.hpp"
#include <variant>
#include <pqxx/pqxx>
#include <fmt/format.h>

using namespace pgsqloo;

struct ConnectionTask : ResultTask<pqxx::connection> {
    std::variant<std::string, pqxx::connection, std::monostate> m_Result = "unknown error";
    std::weak_ptr<Connection> m_Connection;
    std::shared_ptr<PostgreTypes> m_Types;
    GarrysMod::Lua::AutoReference m_ThisRef;

    void GetTypes(pqxx::connection& conn) {
        m_Types = std::make_shared<PostgreTypes>();
        pqxx::work w{ conn };
        for (auto [oid, typcategory] : w.stream<pqxx::oid, std::string_view>("SELECT oid, typcategory FROM pg_type"))
            m_Types->SetType(oid, typcategory.at(0));
    }

    virtual void Process() {
        if (auto conn = m_Connection.lock()) {
            try {
                auto c = pqxx::connection(conn->GetUrl());
                GetTypes(c);
                SetResult(std::move(c));
            } catch (std::exception const& e) {
                SetError(e.what());
            }
        }
        Finish();
    }

    virtual void Finalize(GarrysMod::Lua::ILuaInterface* LUA) {
        if (auto conn = m_Connection.lock()) {
            if (HasResult()) {
                conn->SetConnection(ReleaseResult(), std::move(m_Types));
                SetResult();
            }

            if (m_ThisRef.Push()) {
                LUA->GetField(-1, "OnConnected");
                LUA->Push(-2);
                PushError(LUA);
                utils::RunFunctionWithErrorHandler(LUA, 2, 0);
                LUA->Pop();
            }
        }
    }

    virtual int PushResult(GarrysMod::Lua::ILuaInterface* LUA) {
        LUA->PushNil();
        return 1;
    }
};

struct QueryTask : BasicQueryTask {
    std::string m_SQL;
    std::weak_ptr<Connection> m_Connection;
    GarrysMod::Lua::AutoReference m_CallbackRef;
    bool m_AttemptReconnect = true;
    std::optional<pqxx::params> m_Params;

    virtual void Process() {
        if (auto conn = m_Connection.lock()) {
            try {
                try {
                    auto c = conn->GetConnection();
                    pqxx::nontransaction w{*c};

                    pqxx::result result;
                    if (m_Params) {
                        result = w.exec_params(m_SQL, *m_Params);
                    } else {
                        result = w.exec(m_SQL);
                    }

                    SetResult({ std::move(result), conn->GetTypes() });
                } catch (pqxx::broken_connection const& e) {
                    if (m_AttemptReconnect && conn->TryReconnect()) {
                        m_AttemptReconnect = false;
                        return Process();
                    }
                    throw e;
                }
            } catch (std::exception const& e) {
                SetError(e.what());
            }
        }
        Finish();
    }

    virtual void Finalize(GarrysMod::Lua::ILuaInterface* LUA) {
        if (m_CallbackRef.Push()) {
            PushError(LUA);
            PushResult(LUA);
            utils::RunFunctionWithErrorHandler(LUA, 2, 0);
        }
    }
};

struct PreparedQueryTask : BasicQueryTask {
    std::string m_Name;
    std::weak_ptr<Connection> m_Connection;
    GarrysMod::Lua::AutoReference m_CallbackRef;
    bool m_AttemptReconnect = true;
    pqxx::params m_Params;

    virtual void Process() {
        if (auto conn = m_Connection.lock()) {
            try {
                try {
                    auto c = conn->GetConnection();
                    pqxx::nontransaction w{ *c };
                    auto result = w.exec_prepared(m_Name, m_Params);
                    SetResult({ std::move(result), conn->GetTypes() });
                } catch (pqxx::broken_connection const& e) {
                    if (m_AttemptReconnect && conn->TryReconnect()) {
                        m_AttemptReconnect = false;
                        return Process();
                    }
                    throw e;
                }
            } catch (std::exception const& e) {
                SetError(e.what());
            }
        }
        Finish();
    }

    virtual void Finalize(GarrysMod::Lua::ILuaInterface* LUA) {
        if (m_CallbackRef.Push()) {
            PushError(LUA);
            PushResult(LUA);
            utils::RunFunctionWithErrorHandler(LUA, 2, 0);
        }
    }
};

Connection::Connection(std::string_view url)
    : m_Url(url)
{
    m_Worker = g_Core->CreateWorker();
    m_Worker->Start();
}

Connection::~Connection() {}

std::shared_ptr<IResultTask> Connection::Connect(GarrysMod::Lua::AutoReference&& thisRef) {
    if (IsConnecting()) return m_ConnectionTask.lock();

    Disconnect();

    auto task = g_Core->m_Factory->Make<ConnectionTask>();
    task->m_Connection = weak_from_this();
    task->m_ThisRef = std::move(thisRef);
    m_ConnectionTask = task;

    AddTask(task, ADD_TASK_TO_HEAD);
    return task;
}

void Connection::Disconnect(bool gracefully) {
    m_Worker->Restart(gracefully);
    m_Connection.reset();
}

bool Connection::TryReconnect(int attempts) {
    for (int i = 0; i < attempts; ++i) {
        auto task = std::make_shared<ConnectionTask>();
        task->m_Connection = weak_from_this();
        m_ConnectionTask = task;

        task->Process();
        task->Finalize(nullptr);
        if (IsConnected()) return true;
    }
    return false;
}

void Connection::SetConnection(pqxx::connection&& conn, std::shared_ptr<PostgreTypes>&& types) {
    m_Connection = std::make_shared<pqxx::connection>(std::move(conn));
    m_Types = std::move(types);

    for (const auto& [name, sql] : m_PreparedStatements)
        m_Connection->prepare(name, sql);
}

std::shared_ptr<IResultTask> Connection::Query(std::string_view query, GarrysMod::Lua::AutoReference&& callback, std::optional<pqxx::params>&& params) {
    auto task = g_Core->m_Factory->Make<QueryTask>();
    task->m_CallbackRef = std::move(callback);
    task->m_SQL = query;
    task->m_Connection = weak_from_this();
    task->m_Params = std::move(params);

    AddTask(task);
    return task;
}

std::shared_ptr<IResultTask> Connection::PreparedQuery(std::string_view name, pqxx::params&& params, GarrysMod::Lua::AutoReference&& callback) {
    auto task = g_Core->m_Factory->Make<PreparedQueryTask>();
    task->m_CallbackRef = std::move(callback);
    task->m_Name = name;
    task->m_Connection = weak_from_this();
    task->m_Params = std::move(params);

    AddTask(task);
    return task;
}

void pgsqloo::Connection::Prepare(const std::string& name, const std::string& query) {
    m_PreparedStatements[name] = query;
    if (IsConnected()) m_Connection->prepare(name, query);
}

void Connection::Unprepare(const std::string& name) {
    m_PreparedStatements.erase(name);
    if (IsConnected()) m_Connection->unprepare(name);
}
