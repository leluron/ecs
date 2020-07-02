for i=1, 10 do
    e = createEntity()
    pos = e:createComponent("PositionComponent")
    vel = e:createComponent("VelocityComponent")
    pos.x = i%7; pos.y = i%5; pos.z = i%3;
    vel.x = i%5; vel.y = i%2; vel.z = -i%3;
end
addScript("scripts/test1.lua")