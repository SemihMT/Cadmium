-- ==========================================
-- Cadmium Engine Demo: Neon Swarm Simulator
-- ==========================================

-- Register custom components to store physics and visual data
Component.Register("Velocity", { x = 0.0, y = 0.0 })
Component.Register("ColorData", { r = 1.0, g = 1.0, b = 1.0 })

-- Factory function to generate our swarm entities
local function SpawnParticle(px, py)
    local e = Entity.New("Particle")

    -- Utilize built-in Transform properties
    e.x = px
    e.y = py
    e.scaleX = math.random(2, 5) -- Used here as particle radius

    -- Assign random initial velocities and neon colors
    local angle = math.random() * math.pi * 2
    local speed = math.random(50, 300)

    Component.Add(e, "Velocity", {
        x = math.cos(angle) * speed,
        y = math.sin(angle) * speed
    })

    Component.Add(e, "ColorData", {
        r = math.random(),
        g = math.random(0.5, 1.0), -- Bias towards greens/blues
        b = 1.0
    })

    -- Attach per-entity update logic natively to the handle
    function e:OnUpdate(dt)
        local vel = Component.Get(self, "Velocity")
        local mouse = Input.MousePosition()

        -- Attract to mouse if left click is held
        if Input.IsMouseDown(1) then
            local dx = mouse.x - self.x
            local dy = mouse.y - self.y
            local dist = math.sqrt(dx * dx + dy * dy)

            -- Prevent division by zero and extreme snapping
            if dist > 5 then
                local force = 1500
                vel.x = vel.x + (dx / dist) * force * dt
                vel.y = vel.y + (dy / dist) * force * dt
            end
        end

        -- Apply slight friction to prevent infinite acceleration
        vel.x = vel.x * 0.98
        vel.y = vel.y * 0.98

        -- Integrate velocity
        self.x = self.x + vel.x * dt
        self.y = self.y + vel.y * dt

        -- Screen wrap around using Scene globals
        if self.x < 0 then self.x = Scene.Width end
        if self.x > Scene.Width then self.x = 0 end
        if self.y < 0 then self.y = Scene.Height end
        if self.y > Scene.Height then self.y = 0 end
    end

    -- Attach custom rendering logic
    function e:OnRender()
        local col = Component.Get(self, "ColorData")
        local vel = Component.Get(self, "Velocity")
        local renderColor = Color.RGBA(col.r, col.g, col.b, 0.8)

        -- Draw a line trailing behind the particle based on its velocity vector
        Draw.Line(self.x, self.y, self.x - vel.x * 0.08, self.y - vel.y * 0.08, renderColor)

        -- Draw the particle core
        Draw.FilledCircle(self.x, self.y, self.scaleX, renderColor)
    end
end

-- ==========================================
-- Global Scene Hooks
-- ==========================================

function OnEnter()
    math.randomseed(os.time())

    -- Spawn 500 particles in the center of the screen
    for i = 1, 500 do
        SpawnParticle(Scene.Width / 2, Scene.Height / 2)
    end
end

function OnRender()
    -- Draw a dark background trail to make the neon colors pop
    Draw.FilledRect(0, 0, Scene.Width, Scene.Height, Color.RGBA(0.05, 0.05, 0.08, 1.0))

    -- Draw HUD elements
    Draw.Text("Hold Left Click to Attract Swarm", 20, 20, 24, Color.White)
    Draw.Text("Active Particles: " .. Entity.Count(), 20, 50, 18, Color.Gray(0.7))
end
