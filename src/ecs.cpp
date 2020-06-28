#include <iostream>
#include <memory>
#include <algorithm>
#include <cstring>

#include "ecs.h"
#include "component.h"
#include "generated/processes_include.h"

using namespace std;

lua_State* ECS::loadLuaFile(const char* filename) {
    auto L = luaL_newstate();
    luaL_openlibs(L);
    // Load file
    auto code = luaL_loadfile(L, filename);
    if (code != LUA_OK) {
        cerr << "Error loading " <<
            ":" << lua_tostring(L, -1) << endl;
        lua_close(L);
    }
    // Define "createEntity" function
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        auto pthis = static_cast<ECS*>(lua_touserdata(L, lua_upvalueindex(1)));
        // Create new entity
        auto id = pthis->createEntityId();
        pthis->luapushEntity(L, id);
        return 1;
    }, 1);
    lua_setglobal(L, "createEntity");
    // Define "exitScript" function
    lua_pushboolean(L, 0);
    lua_setglobal(L, "exitScript__");
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushboolean(L, 1);
        lua_setglobal(L, "exitScript__");
        return 0;
    });
    lua_setglobal(L, "exitScript");
    // Run script
    lua_call(L, 0, 0);
    return L;
}

entity_id ECS::createEntityId() {
    entity_id id;
    if (deadEntities.size()) {
        id = deadEntities.back();
        deadEntities.pop_back();
    } else {
        id = next;
        next += 1;
    }
    bornEntities.push_back(id);
    return id;
}

Entity ECS::createEntity() {
    return Entity(createEntityId(), *this);
}

void ECS::destroy(entity_id e) {
    shotEntities.push_back(e);
}

void ECS::init(const char* initFile) {
#define X(name) processes[#name] = shared_ptr<Process>(new name(*this));
#include "generated/processes.def"
#undef X

    loadLuaFile(initFile);
}

bool ECS::step() {
    bool exiting = false;
    // Born become live
    liveEntities.insert(
        liveEntities.end(), 
        bornEntities.begin(), 
        bornEntities.end()
    );
    bornEntities.clear();
    // Run scripts
    for (auto s : scripts) {
        auto id = s.first;
        auto &list = s.second;
        for (auto &L : list) {
            lua_getglobal(L, "process");
            luapushEntity(L, id);
            lua_call(L, 1, 0);
            lua_getglobal(L, "exitScript__");
            auto quit = lua_tonumber(L, -1);
            lua_pop(L, 1);
            if (quit) {
                lua_close(L);
                L = nullptr;
            }
        }
        list.erase(std::remove_if(list.begin(), list.end(),
            [](auto l){return l;}), list.end());

    }
    // Run processes
    for (auto it : processes) {
        auto p = it.second; 
        for (auto e : liveEntities) {
            auto entity = Entity(e, *this);
            // Select entity and process it
            if (p->allow(entity))
                p->process(entity);
        }
        exiting |= p->done();
    }
    // Remove components from shot entities
    #define X(name) removeComponent<name>(e);
    for (auto e : shotEntities) {
        #include "generated/components.def"
    }
    #undef X
    // Shot become dead
    deadEntities.insert(
        deadEntities.end(), 
        shotEntities.begin(), 
        shotEntities.end()
    );

    return !exiting;
}

void ECS::addScript(entity_id e, const char* filename) {
    auto L = loadLuaFile(filename);
    lua_getglobal(L, "process");
    if (lua_isfunction(L, -1)) {
        scripts[e].push_back(L);
    }
    lua_pop(L, 1);
}

void ECS::luapushEntity(lua_State *L, entity_id id) {
    lua_newtable(L);
    // Store id in entity
    lua_pushstring(L, "id");
    lua_pushnumber(L, id);
    lua_rawset(L, -3);
    // Set createComponent function
    lua_pushstring(L, "createComponent");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        auto pthis = static_cast<ECS*>(lua_touserdata(L, lua_upvalueindex(1)));
        auto comp_type = lua_tostring(L, -1);
        lua_pushstring(L, "id");
        lua_rawget(L, -3);
        auto id = lua_tonumber(L, -1);
        lua_pop(L, 3);
        Component *c = nullptr;
        #define X(name) if (!strcmp(comp_type, #name)) c = static_cast<Component*>(&pthis->createComponent<name>(id));
        #include "generated/components.def"
        #undef X
        if (!c) throw runtime_error("Unknown component");
        lua_newtable(L);
        lua_newtable(L);
        c->tolua(L);
        lua_setmetatable(L, -2);
        return 1;
    }, 1);
    lua_settable(L, -3);
    // Set getComponent function
    lua_pushstring(L, "getComponent");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        auto pthis = static_cast<ECS*>(lua_touserdata(L, lua_upvalueindex(1)));
        auto comp_type = lua_tostring(L, -1);
        lua_pushstring(L, "id");
        lua_rawget(L, -3);
        auto id = lua_tonumber(L, -1);
        lua_pop(L, 3);
        Component *c = nullptr;
        #define X(name) if (!strcmp(comp_type, #name)) c = static_cast<Component*>(&pthis->getComponent<name>(id));
        #include "generated/components.def"
        #undef X
        if (!c) throw runtime_error("Unknown component");
        lua_newtable(L);
        lua_newtable(L);
        c->tolua(L);
        lua_setmetatable(L, -2);
        return 1;
    }, 1);
    lua_settable(L, -3);
    // Set hasComponent function
    lua_pushstring(L, "hasComponent");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        auto pthis = static_cast<ECS*>(lua_touserdata(L, lua_upvalueindex(1)));
        auto comp_type = lua_tostring(L, -1);
        lua_pushstring(L, "id");
        lua_rawget(L, -3);
        auto id = lua_tonumber(L, -1);
        lua_pop(L, 3);
        int has = -1;
        #define X(name) if (!strcmp(comp_type, #name)) has = pthis->hasComponent<name>(id);
        #include "generated/components.def"
        #undef X
        if (has==-1) throw runtime_error("Unknown component");
        lua_pushboolean(L, has);
        return 1;
    }, 1);
    lua_settable(L, -3);
    // Set removeComponent function
    lua_pushstring(L, "removeComponent");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        auto pthis = static_cast<ECS*>(lua_touserdata(L, lua_upvalueindex(1)));
        auto comp_type = lua_tostring(L, -1);
        lua_pushstring(L, "id");
        lua_rawget(L, -3);
        auto id = lua_tonumber(L, -1);
        lua_pop(L, 3);
        #define X(name) if (!strcmp(comp_type, #name)) pthis->removeComponent<name>(id);
        #include "generated/components.def"
        #undef X
        return 0;
    }, 1);
    lua_settable(L, -3);
    // Set destroy function
    lua_pushstring(L, "destroy");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        auto pthis = static_cast<ECS*>(lua_touserdata(L, lua_upvalueindex(1)));
        lua_pushstring(L, "id");
        lua_rawget(L, -2);
        auto id = lua_tonumber(L, -1);
        lua_pop(L, 2);
        pthis->destroy(id);
        return 0;
    }, 1);
    lua_settable(L, -3);
    // Set addScript function
    lua_pushstring(L, "addScript");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        auto pthis = static_cast<ECS*>(lua_touserdata(L, lua_upvalueindex(1)));
        auto filename = lua_tostring(L, -1);
        lua_pushstring(L, "id");
        lua_rawget(L, -3);
        auto id = lua_tonumber(L, -1);
        lua_pop(L, 3);
        pthis->addScript(id, filename);
        return 0;
    }, 1);
    lua_settable(L, -3);
}

bool Process::done() {
    return _exit;
}

void Process::exit() {
    _exit = true;
}

Entity Process::createEntity() {
    return ecs.createEntity();
}

int main(int argc, char **argv) {
    if (argc > 2) {
        cerr << "Unknown arguments" << endl;
        return -1;
    }
    ECS ecs;
    ecs.init((argc==2)?argv[1]:"scripts/init.lua");
    ecs.step();
    return 0;
}
