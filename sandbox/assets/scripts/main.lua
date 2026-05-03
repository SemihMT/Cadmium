-- assets/scripts/main.lua
-- Phase 2 validation — tests Entity.New, hooks, Transform proxy,
-- Entity.Find, Entity.FindAll, Entity.Destroy

-- ── Test 1: basic entity with transform proxy ─────────────────────────────
local ball = Entity.New("Ball")
ball.x      = Scene.Width / 2
ball.y      = Scene.Height / 2
ball.vx     = 220
ball.vy     = 160
ball.radius = 24
ball.color  = Color.Red
ball.tag    = "ball"

function ball:OnUpdate(dt)
    -- Transform proxy writes go directly to C++ Transform
    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt

    -- Bounce
    if self.x - self.radius < 0 then
        self.x  = self.radius
        self.vx = math.abs(self.vx)
    elseif self.x + self.radius > Scene.Width then
        self.x  = Scene.Width - self.radius
        self.vx = -math.abs(self.vx)
    end

    if self.y - self.radius < 0 then
        self.y  = self.radius
        self.vy = math.abs(self.vy)
    elseif self.y + self.radius > Scene.Height then
        self.y  = Scene.Height - self.radius
        self.vy = -math.abs(self.vy)
    end

    -- Press Space to randomise
    if Input.IsKeyJustPressed("Space") then
        self.vx    = math.random(-300, 300)
        self.vy    = math.random(-200, 200)
        self.color = Color.RGBA(math.random(), math.random(), math.random(), 1)
    end
end

function ball:OnRender()
    Draw.FilledCircle(self.x, self.y, self.radius, self.color)
    Draw.Circle(self.x, self.y, self.radius + 2, Color.White)
end

-- ── Test 2: multiple entities, FindAll, Destroy ───────────────────────────
local function MakeDot(x, y)
    local d = Entity.New()
    d.x      = x
    d.y      = y
    d.vy     = 30 + math.random() * 60
    d.life   = 2 + math.random() * 2
    d.color  = Color.RGBA(math.random(), math.random(), 1, 1)
    d.tag    = "dot"

    function d:OnUpdate(dt)
        self.y    = self.y + self.vy * dt
        self.life = self.life - dt

        if self.life <= 0 then
            Entity.Destroy(self)
        end
    end

    function d:OnRender()
        local alpha = math.max(0, self.life / 3)
        Draw.FilledCircle(self.x, self.y, 5,
            Color.RGBA(self.color.r, self.color.g, self.color.b, alpha))
    end

    return d
end

-- Spawn dots periodically
local spawnTimer = 0
local spawnRate  = 0.15  -- seconds between spawns

-- ── Test 3: Entity.Find ───────────────────────────────────────────────────
local function MakeHUD()
    local hud = Entity.New("HUD")

    function hud:OnUpdate(dt)
        -- Test Entity.Find
        local b = Entity.Find("Ball")
        if b then
            self.ballX = b.x
            self.ballY = b.y
        end

        -- Test Entity.FindAll
        local dots = Entity.FindAll("dot")
        self.dotCount = #dots
    end

    function hud:OnRender()
        Draw.Text("Phase 2 validation", 10, 10, 16, Color.White)
        Draw.Text("Space: randomise ball", 10, 30, 14, Color.Gray(0.7))
        Draw.Text("Entities: " .. Entity.Count(), 10, 50, 14, Color.Yellow)

        if self.dotCount then
            Draw.Text("Dots: " .. self.dotCount, 10, 70, 14, Color.Cyan)
        end

        if self.ballX then
            Draw.Text(
                string.format("Ball: %.0f, %.0f", self.ballX, self.ballY),
                10, 90, 14, Color.Gray(0.7)
            )
        end
    end

    return hud
end

local hud = MakeHUD()

-- ── Scene-level OnUpdate for spawning ────────────────────────────────────
function OnUpdate(dt)
    spawnTimer = spawnTimer + dt
    if spawnTimer >= spawnRate then
        spawnTimer = 0
        MakeDot(math.random(50, Scene.Width - 50), 0)
    end
end

-- ── Scene-level OnEnter ───────────────────────────────────────────────────
function OnEnter()
    math.randomseed(os.time())
    -- Verify Entity.Find works at startup
    local b = Entity.Find("Ball")
    if b then
        SDL_Log = SDL_Log or print
        print("Entity.Find('Ball') OK — found at " .. b.x .. ", " .. b.y)
    else
        print("Entity.Find('Ball') FAILED")
    end
end
