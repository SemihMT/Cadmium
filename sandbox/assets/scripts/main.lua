--  Assets
local playerTex = Assets.LoadTexture("sprites/ship.png")
local enemyTex  = Assets.LoadTexture("sprites/barrel.png")

--  Components
Component.Register("Health", {
    hp    = 100.0,
    maxHp = 100.0,
    alive = true
})

--  Player
local Player = Entity.New("Player")
Player.tag = "Player"
Player.x   = Scene.Width / 2
Player.y   = Scene.Height - 80
Component.Add(Player, "Health", { hp = 100, maxHp = 100 })

function Player:OnUpdate(dt)
    local speed = 300
    if Input.IsKeyDown("Left")  or Input.IsKeyDown("A") then self.x = self.x - speed * dt end
    if Input.IsKeyDown("Right") or Input.IsKeyDown("D") then self.x = self.x + speed * dt end
    if Input.IsKeyDown("Up")    or Input.IsKeyDown("W") then self.y = self.y - speed * dt end
    if Input.IsKeyDown("Down")  or Input.IsKeyDown("S") then self.y = self.y + speed * dt end

    self.x = math.clamp(self.x, 0, Scene.Width  - 40)
    self.y = math.clamp(self.y, 0, Scene.Height - 40)

    local hp = Component.Get(self, "Health")
    if hp and hp.hp <= 0 then
        hp.alive = false
        Entity.Destroy(self)
    end

    local enemies = Entity.FindAll("Enemy")
    for _, enemy in ipairs(enemies) do
        local dx   = self.x - enemy.x
        local dy   = self.y - enemy.y
        local dist = math.sqrt(dx * dx + dy * dy)
        if dist < 30 then
            local eHp = Component.Get(enemy, "Health")
            if eHp then eHp.hp = 0 end
            local pHp = Component.Get(self, "Health")
            if pHp then pHp.hp = pHp.hp - 15 end
        end
    end
end

function Player:OnRender()
    if not Assets.IsValid(playerTex) then
        Draw.FilledRect(self.x, self.y, 40, 40, Color.Magenta)
    else
        Draw.Sprite(playerTex, self.x, self.y, 40, 40, 0.0, Color.White)
    end
end

--  Enemy factory
local function SpawnBarrel()
    local e = Entity.New()
    e.tag = "Enemy"
    e.x   = math.random(50, Scene.Width - 50)
    e.y   = -30
    Component.Add(e, "Health", { hp = 30, maxHp = 30 })

    function e:OnUpdate(dt)
        self.y = self.y + 120 * dt
        if self.y > Scene.Height + 30 then
            self.y = -30
            self.x = math.random(50, Scene.Width - 50)
        end
        local hp = Component.Get(self, "Health")
        if hp and hp.hp <= 0 then
            Entity.Destroy(self)
        end
    end

    function e:OnRender()
        if not Assets.IsValid(enemyTex) then
            Draw.FilledRect(self.x, self.y, 32, 32, Color.Red)
        else
            Draw.Sprite(enemyTex, self.x, self.y, 32, 32, 0.0, Color.White)
        end
    end
end

--  Scene OnUpdate
local spawnTimer    = 0
local spawnInterval = 1.5

function OnUpdate(dt)
    spawnTimer = spawnTimer + dt
    if spawnTimer > spawnInterval then
        spawnTimer = spawnTimer - spawnInterval
        SpawnBarrel()
    end
end

--  Scene OnRender
function OnRender()
    Draw.FilledRect(0, 0, Scene.Width, Scene.Height, Color.Gray(0.1))

    local hpComp = Component.Get(Player, "Health")
    if hpComp then
        Draw.Text("HP: " .. math.floor(hpComp.hp), 10, 10, 24, Color.White)
    end
    Draw.Text("Entities: " .. Entity.Count(), 10, 40, 18, Color.Gray(0.7))
end

--  Scene OnFixedUpdate
function OnFixedUpdate(dt)
    if Input.IsKeyJustPressed("Space") then
        local enemies = Entity.FindAll("Enemy")
        for _, e in ipairs(enemies) do
            Entity.Destroy(e)
        end
    end
end

--  Scene OnEnter
function OnEnter()
    math.randomseed(os.time())
end
