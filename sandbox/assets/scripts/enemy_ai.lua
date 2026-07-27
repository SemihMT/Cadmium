Name = "EnemyAI"

counter = 0
speed = 20
target = nil

function OnStart()
    target = FindEntityByTag("Player")
    if target then
        print(Name .. ": found target")
    else
        print(Name .. ": no target found, will stay idle")
    end
end

function OnUpdate()
    counter = counter + 1

    if target and not target:IsValid() then
        target = nil
    end

    if target then
        local myPos = self:GetTransform()
        local targetPos = target:GetTransform()

        if targetPos.x > myPos.x then
            myPos.x = myPos.x + speed * Time.dt
        elseif targetPos.x < myPos.x then
            myPos.x = myPos.x - speed * Time.dt
        end
    end

    if counter % 25 == 0 then
        local health = self:GetScript("Health")
        if health and health.currentHP > 0 then
            health.TakeDamage(1)
            if health.currentHP < 50 then
                print(Name .. ": low health, retreating logic could go here")
            end
        end
    end
end

function OnDestroy()
    print(Name .. ": OnDestroy called!")
end
