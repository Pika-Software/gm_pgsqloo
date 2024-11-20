#include <tuple>
#include <vector>

#include "async_postgres.hpp"

struct FieldInfo {
    const char* name;
    bool text;
    Oid type;
};

void async_postgres::create_result_table(GLua::ILuaInterface* lua,
                                         PGresult* result) {
    lua->CreateTable();

    std::vector<FieldInfo> fields;

    // Fields metadata
    lua->CreateTable();
    int nFields = PQnfields(result);
    for (int i = 0; i < nFields; i++) {
        lua->PushNumber(i + 1);

        FieldInfo info = {PQfname(result, i), PQfformat(result, i) == 0,
                          PQftype(result, i)};

        lua->CreateTable();
        lua->PushString(info.name);
        lua->SetField(-2, "name");

        lua->PushNumber(info.type);
        lua->SetField(-2, "type");

        lua->SetTable(-3);
        fields.push_back(info);
    }
    lua->SetField(-2, "fields");

    // Rows
    lua->CreateTable();
    int nTuples = PQntuples(result);
    for (int i = 0; i < nTuples; i++) {
        lua->PushNumber(i + 1);
        lua->CreateTable();
        for (int j = 0; j < nFields; j++) {
            lua->PushString(fields[j].name);
            if (!PQgetisnull(result, i, j)) {
                if (fields[j].text) {
                    lua->PushString(PQgetvalue(result, i, j));
                } else {
                    lua->PushString(PQgetvalue(result, i, j),
                                    PQgetlength(result, i, j));
                }
            } else {
                lua->PushNil();
            }
            lua->SetTable(-3);
        }
        lua->SetTable(-3);
    }
    lua->SetField(-2, "rows");

    lua->PushString(PQcmdStatus(result));
    lua->SetField(-2, "command");

    lua->PushNumber(PQoidValue(result));
    lua->SetField(-2, "oid");
}
