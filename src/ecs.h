#pragma once

#include "generated/components_include.h"

#include "component.h"
#include "process.h"

#include <typeindex>
#include <unordered_map>
#include <memory>
#include <lua.hpp>
#include <vector>

using entity_id = uint32_t;

class Entity;

class ECS {
public:
    void init(const char* initFile);
    bool step();

    Entity createEntity();
    entity_id createEntityId();

    void luapushEntity(lua_State *L, entity_id id);
    template <class T>
    T& createComponent(entity_id e) {
        auto& a = bag<T>();
        a[e] = T();
        return a[e];
    }
    template <class T>
    T& getComponent(entity_id e) {
        return bag<T>()[e];
    }
    template <class T>
    bool hasComponent(entity_id e) {
        return bag<T>().find(e) != bag<T>().end();
    }
    template <class T>
    void removeComponent(entity_id e) {
        bag<T>().erase(e);
    }
    void destroy(entity_id e);

    void addScript(const char* filename);

private:
    lua_State* loadLuaFile(const char* filename);
    template <class T>
    std::unordered_map<entity_id, T>& bag() {
        auto it = components.find(typeid(T));
        if (it == components.end()) {
            auto a = new std::unordered_map<entity_id, T>();
            components[typeid(T)] = std::shared_ptr<void>(a);
            return *a;
        } else {
            return *std::static_pointer_cast<std::unordered_map<entity_id, T>>(it->second);
        }
    }

    std::unordered_map<
        const char*,
        std::shared_ptr<Process>
    > processes;

    std::unordered_map<
        std::type_index,
        std::shared_ptr<void>
    > components;

     std::vector<lua_State*> scripts;

    std::vector<entity_id> 
        bornEntities,
        liveEntities,
        shotEntities,
        deadEntities;
    entity_id next = 1;
};

class Entity {
public:
    Entity(entity_id id, ECS &ecs) : id{id}, ecs{ecs} {}
    template <class T>
    T& createComponent() { return ecs.createComponent<T>(id); }
    template <class T>
    T& getComponent() { return ecs.getComponent<T>(id); }
    template <class T>
    bool hasComponent() { return ecs.hasComponent<T>(id); }
    template <class T>
    void removeComponent() { ecs.removeComponent<T>(id); }
    void destroy() { ecs.destroy(id); }
private:
    entity_id id;
    ECS &ecs;
};