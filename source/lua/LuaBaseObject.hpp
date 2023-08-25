#pragma once

#include "Core.hpp"

#include <GarrysMod/Lua/AutoReference.h>
#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaInterface.h>

namespace pgsqloo {
    template <class T>
    class LuaBaseObject {
        GarrysMod::Lua::AutoReference m_TableRef;

    public:
        LuaBaseObject() {
            g_Core->LUA->CreateTable();
            m_TableRef = GarrysMod::Lua::AutoReference(g_Core->LUA);
        }

        template <class... Args>
        static T* CreateAndPush(Args&&... args) {
            T* obj = new T(std::forward<Args>(args)...);
            obj->PushFromLua();
            return obj;
        }
        void PushFromLua() {
            g_Core->LUA->PushUserType(this, T::MetaType);
            if (g_Core->LUA->PushMetaTable(T::MetaType))
                g_Core->LUA->SetMetaTable(-2);
        }
        static T* GetFromLua(int iStackPos = 1) {
            return g_Core->LUA->GetUserType<T>(iStackPos, T::MetaType);
        }
        static T* CheckFromLua(int iStackPos = 1) {
            g_Core->LUA->CheckType(iStackPos, T::MetaType);
            return GetFromLua(iStackPos);
        }
        bool PushTable() { return m_TableRef.Push(); }

    protected:
        LUA_FUNCTION_STATIC_MEMBER(HandleGC) {
            auto obj = CheckFromLua(1);
            if (obj) delete obj;
            return 0;
        }
        LUA_FUNCTION_STATIC_MEMBER(HandleGetTable) {
            auto obj = CheckFromLua(1);
            obj->PushTable();
            return 1;
        }
        LUA_FUNCTION_STATIC_MEMBER(HandleIndex) {
            auto obj = CheckFromLua(1);
            if (obj->PushTable()) {
                LUA->Push(2);
                LUA->GetTable(-2);
                if (!LUA->IsType(-1, GarrysMod::Lua::Type::Nil)) return 1;
            }

            LUA->PushMetaTable(T::MetaType);
            LUA->Push(2);
            LUA->GetTable(-2);
            return 1;
        }
        LUA_FUNCTION_STATIC_MEMBER(HandleNewIndex) {
            auto obj = CheckFromLua(1);
            if (obj->PushTable()) {
                LUA->Push(2);
                LUA->Push(3);
                LUA->SetTable(-3);
            }
            return 0;
        }
    };
}  // namespace pgsqloo
