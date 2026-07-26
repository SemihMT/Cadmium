function OnStart(self)
    -- runs once, before the first OnUpdate
    print("OnStart called!")
end

counter = 0
function OnUpdate(self, dt)
    -- runs every frame
    counter = counter + 1
    print("OnUpdate called for the", counter, "th time!")
end

function OnDestroy(self)
    -- runs once when the entity is destroyed mid-scene or when the scene itself is destroyed
    print("OnDestroy called!")
end
