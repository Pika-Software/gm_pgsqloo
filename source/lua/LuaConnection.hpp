#pragma once

#include "lua/LuaBaseObject.hpp"
#include <string_view>
#include <memory>

namespace pgsqloo {
    class Connection;

    struct LuaConnection : public LuaBaseObject<LuaConnection> {
        std::shared_ptr<Connection> m_Inner;

        LuaConnection(std::string_view url);
        ~LuaConnection();

        static int MetaType;
        static void Initialize(GarrysMod::Lua::ILuaInterface* LUA);
    };
}  // namespace pgsqloo