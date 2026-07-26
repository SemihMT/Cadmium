function OnStart(self)
    -- runs once, before the first OnUpdate
    print("OnStart called!")
end

function OnUpdate(self,dt)
    local t = self:GetTransform()
    t.x = t.x + 10 * Time.dt
end

function OnDestroy(self)
    -- runs once when the entity is destroyed mid-scene or when the scene itself is destroyed
    print("OnDestroy called!")
end
