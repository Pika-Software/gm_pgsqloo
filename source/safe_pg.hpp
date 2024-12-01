#pragma once

#include <libpq-fe.h>

#include <memory>
#include <string_view>

/// Just small wrappers around libpq types to not have to deal with freeing them
/// manually.
namespace async_postgres::pg {
    using conn = std::unique_ptr<PGconn, decltype(&PQfinish)>;
    using result = std::unique_ptr<PGresult, decltype(&PQclear)>;
    using notify = std::unique_ptr<PGnotify, decltype(&PQfreemem)>;

    inline conn connectStart(std::string_view conninfo) {
        return conn(PQconnectStart(conninfo.data()), &PQfinish);
    }

    inline result getResult(conn& conn) {
        return result(PQgetResult(conn.get()), &PQclear);
    }

    inline notify getNotify(conn& conn) {
        return notify(PQnotifies(conn.get()), &PQfreemem);
    }
}  // namespace async_postgres::pg
