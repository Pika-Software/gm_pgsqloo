#pragma once

#include "ITask.hpp"
#include "Worker.hpp"
#include "PostgreTypes.hpp"

#include <pqxx/connection>
#include <memory>
#include <string_view>
#include <GarrysMod/Lua/AutoReference.h>
#include <unordered_map>
#include <optional>

namespace pgsqloo {
    class Connection : public std::enable_shared_from_this<Connection> {
        std::string m_Url;
        std::shared_ptr<pqxx::connection> m_Connection;
        std::shared_ptr<Worker> m_Worker;
        std::shared_ptr<PostgreTypes> m_Types;
        std::unordered_map<std::string, std::string> m_PreparedStatements;
        
        std::weak_ptr<IResultTask> m_ConnectionTask;

    public:
        Connection(std::string_view url);
        ~Connection();

        const std::string& GetUrl() const { return m_Url; }
        std::shared_ptr<PostgreTypes> GetTypes() const { return m_Types; }
        std::shared_ptr<pqxx::connection> GetConnection() const {
            if (!m_Connection) throw pqxx::broken_connection();
            return m_Connection;
        }
        bool IsConnected() const { return m_Connection && m_Connection->is_open(); }
        bool IsConnecting() const { return !IsConnected() && !m_ConnectionTask.expired(); }

        void AddTask(std::shared_ptr<ITask> task, AddTaskTo_t to = ADD_TASK_TO_TAIL) { m_Worker->AddWorkerTask(task, to); }

        std::shared_ptr<IResultTask> Connect(GarrysMod::Lua::AutoReference&& thisRef = {});
        void Disconnect(bool gracefully = false);
        bool TryReconnect(int attempts = 3);
        void SetConnection(pqxx::connection&& conn, std::shared_ptr<PostgreTypes>&& types);

        std::shared_ptr<IResultTask> Query(std::string_view query, GarrysMod::Lua::AutoReference&& callback = {}, std::optional<pqxx::params>&& params = {});
        std::shared_ptr<IResultTask> PreparedQuery(std::string_view name, pqxx::params&& params, GarrysMod::Lua::AutoReference&& callback = {});

        void Prepare(const std::string& name, const std::string& query);
        void Unprepare(const std::string& name);
    };
}
