Name = "Health"

maxHP = 100
currentHP = 100

function OnStart()
    print(Name .. ": starting with " .. currentHP .. "/" .. maxHP .. " HP")
end

function OnUpdate()
end

function OnDestroy()
    print(Name .. ": OnDestroy called, final HP was " .. currentHP)
end

function TakeDamage(amount)
     if currentHP <= 0 then
        return
    end
    currentHP = currentHP - amount
    print(Name .. ": took " .. amount .. " damage, now at " .. currentHP)
    if currentHP <= 0 then
        currentHP = 0
        print(Name .. ": entity died")
    end
end
