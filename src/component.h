#pragma once

#include <lua.hpp>

class Component {
public:
    virtual void tolua(lua_State *L)=0;
};