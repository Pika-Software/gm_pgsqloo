#pragma once

#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaInterface.h>

#include <algorithm>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>

#define SAFE_LUA_FUNCTION(FUNC)                                           \
    int FUNC##__Imp(GarrysMod::Lua::ILuaInterface* LUA);                  \
    int FUNC(lua_State* L) {                                              \
        GarrysMod::Lua::ILuaInterface* LUA =                              \
            reinterpret_cast<GarrysMod::Lua::ILuaInterface*>(L->luabase); \
        LUA->SetState(L);                                                 \
        try {                                                             \
            return FUNC##__Imp(LUA);                                      \
        } catch (std::exception const& e) {                               \
            LUA->ThrowError(e.what());                                    \
            return 0;                                                     \
        }                                                                 \
    }                                                                     \
    int FUNC##__Imp([[maybe_unused]] GarrysMod::Lua::ILuaInterface* LUA)

#define LUA_PROXY_MEMBER_FUNC(CLASS, FUNC) \
    SAFE_LUA_FUNCTION(CLASS##_##FUNC) {    \
        auto conn = CLASS::CheckFromLua(); \
        return conn->FUNC(LUA);            \
    }

#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return { f }; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

class Color {
public:
    unsigned char r, g, b, a;
    Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
        : r(r), g(g), b(b), a(a) {}
};

namespace pgsqloo::utils {
    inline std::string_view GetString(GarrysMod::Lua::ILuaBase* LUA, int iStackPos = -1) {
        unsigned int size = 0;
        const char* str = LUA->GetString(iStackPos, &size);
        return std::string_view(str, size);
    }
    inline std::string_view CheckString(GarrysMod::Lua::ILuaBase* LUA,
                                        int iStackPos = -1) {
        LUA->CheckType(iStackPos, GarrysMod::Lua::Type::String);
        return GetString(LUA, iStackPos);
    }
    inline std::optional<std::string_view> OptString(
        GarrysMod::Lua::ILuaBase* LUA, int iStackPos = -1) {
        if (LUA->IsType(iStackPos, GarrysMod::Lua::Type::String)) {
            return GetString(LUA, iStackPos);
        }
        return std::nullopt;
    }

    inline std::optional<double> OptNumber(GarrysMod::Lua::ILuaBase* LUA,
                                           int iStackPos = -1) {
        if (LUA->IsType(iStackPos, GarrysMod::Lua::Type::Number)) {
            return LUA->GetNumber(iStackPos);
        }
        return std::nullopt;
    }

    inline void SetFunction(GarrysMod::Lua::ILuaBase* LUA,
                            GarrysMod::Lua::CFunc func, const char* field,
                            int iStackPos = -1) {
        LUA->PushCFunction(func);
        LUA->SetField(iStackPos > 0 ? iStackPos : iStackPos - 1, field);
    }

    inline void SetNumber(GarrysMod::Lua::ILuaBase* LUA,
                            double number, const char* field,
                            int iStackPos = -1) {
        LUA->PushNumber(number);
        LUA->SetField(iStackPos > 0 ? iStackPos : iStackPos - 1, field);
    }

    inline void PushString(GarrysMod::Lua::ILuaBase* LUA, std::string_view str) {
        LUA->PushString(str.data(), (unsigned int)str.size());
    }

    inline std::string UrlEncode(std::string_view value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value) {
            // Keep alphanumeric and other accepted characters intact
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
                continue;
            }

            // Any other characters are percent-encoded
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }

        return std::move(escaped.str());
    }

    // If run failed, then return string. Otherwise returns nullopt
    inline std::optional<std::string> RunFunction(GarrysMod::Lua::ILuaBase* LUA,
                                                  int iArgs, int iReturns) {
        if (LUA->PCall(iArgs, iReturns, 0) != 0) {
            std::string err = std::string{GetString(LUA)};
            LUA->Pop();
            return err;
        }
        return std::nullopt;
    }

    inline int EmptyFunc(lua_State* L) { return 0; }

    // Returns true on sucess
    inline bool RunFunctionWithErrorHandler(GarrysMod::Lua::ILuaInterface* LUA,
                                            int iArgs, int iReturns,
                                            bool silent = false) {
        if (silent) {
            LUA->PushCFunction(EmptyFunc);
        } else {
            LUA->GetField(GarrysMod::Lua::INDEX_GLOBAL, "ErrorNoHaltWithStack");
        }

        LUA->Insert(-2 - iArgs);
        if (LUA->PCall(iArgs, iReturns, -2 - iArgs) != 0) {
            LUA->Pop(2);
            return false;
        }
        LUA->Pop();
        return true;
    }

    inline std::optional<int> FindType(GarrysMod::Lua::ILuaBase* LUA, int iType, const std::vector<int>& iStackPos) {
        for (int pos : iStackPos) {
            if (LUA->IsType(pos, iType)) {
                return pos;
            }
        }
        return {};
    }
}  // namespace pgsqloo::utils
