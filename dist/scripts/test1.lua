function allow(e)
    return e:hasComponent("PositionComponent")
end

function process(e)
    pos = e:getComponent("PositionComponent")
    pos.z = 100
end