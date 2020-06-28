#pragma once

#include <sstream>
#include <string>

#include "../component.h"

class VelocityComponent : public Component {
public:
    float x,y,z;
    std::string toString() {
        std::stringstream ss;
        ss << "x:" << x << " y:" << y << " z:" << z;
        return ss.str();
    }
    void tolua(lua_State *L) {
        lua_pushstring(L, "__index");
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, [](lua_State *L) -> int {
            auto pthis = static_cast<VelocityComponent*>(lua_touserdata(L, lua_upvalueindex(1)));
            auto field = lua_tostring(L, -1);
            lua_pop(L, 2);
            if (!strcmp(field,"x")) lua_pushnumber(L, pthis->x);
            else if (!strcmp(field,"y")) lua_pushnumber(L, pthis->y);
            else if (!strcmp(field, "z")) lua_pushnumber(L, pthis->z);
            else throw std::runtime_error("Unknown field");
            return 1;
        }, 1);
        lua_rawset(L, -3);
        lua_pushstring(L, "__newindex");
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, [](lua_State *L) -> int {
            auto pthis = static_cast<VelocityComponent*>(lua_touserdata(L, lua_upvalueindex(1)));
            auto field = lua_tostring(L, -2);
            auto val = lua_tonumber(L, -1);
            lua_pop(L, 3);
            if (!strcmp(field,"x")) pthis->x = val;
            else if (!strcmp(field,"y")) pthis->y = val;
            else if (!strcmp(field,"z")) pthis->z = val;
            else throw std::runtime_error("Unknown field");
            return 0;
        }, 1);
        lua_rawset(L, -3);
    } 
};