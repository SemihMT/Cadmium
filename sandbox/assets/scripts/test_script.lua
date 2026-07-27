Name = "TestScript"

counter = 0

function OnStart()
    print(Name .. ": OnStart called!")
end

function OnUpdate()
    counter = counter + 1
    print(Name .. ": OnUpdate called for the " .. counter .. "th time!")

    local t = self:GetTransform()
    t.x = t.x + 10 * Time.dt

    if counter >= 100 then
        self:Destroy()
    end
end

function OnDestroy()
    print(Name .. ": OnDestroy called!")
end
