#pragma once

class Entity;

class ECS;

class Process {

public:
    Process(ECS &ecs) : ecs{ecs} {}
    virtual bool allow(Entity &e) = 0;
    virtual void process(Entity &e) = 0;
    bool done();

protected:
    virtual void exit();
    Entity createEntity();

private:
    ECS &ecs;
    bool _exit = false;
};