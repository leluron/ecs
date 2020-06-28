#pragma once
#include <iostream>

class IntegrationProcess : public Process {
public:
    using Process::Process;
    virtual bool allow(Entity &e) {
        return e.hasComponent<PositionComponent>() && 
            e.hasComponent<VelocityComponent>();
    }

    virtual void process(Entity &e) {
        auto &pos = e.getComponent<PositionComponent>();
        auto &vel = e.getComponent<VelocityComponent>();
        pos.x += vel.x;
        pos.y += vel.y;
        pos.z += vel.z;
        std::cout << pos.toString() << std::endl;
        std::cout << vel.toString() << std::endl;
    }
};