-- =============================================================================
-- boids_demo.lua — Boids flocking simulation for Cadmium
-- Controls:
--   Mouse        — attracts flock passively / hold LMB to repel
--   Up/Down      — increase / decrease boid count
--   Left/Right   — decrease / increase simulation speed
-- =============================================================================

-- ── Constants ─────────────────────────────────────────────────────────────────

local BOID_COUNT_START = 120
local BOID_COUNT_MAX   = 300
local BOID_COUNT_MIN   = 10

local RADIUS_SEPARATION = 25.0
local RADIUS_ALIGNMENT  = 55.0
local RADIUS_COHESION   = 80.0

local W_SEPARATION = 1.2
local W_ALIGNMENT  = 0.8
local W_COHESION   = 1.0

local SPEED_MIN   = 80.0
local SPEED_MAX   = 160.0
local SPEED_SCALE = 1.0

local MOUSE_RADIUS    = 180.0
local W_MOUSE_ATTRACT = 2.5
local W_MOUSE_REPEL   = 4.0

local BOID_LENGTH = 10.0
local BOID_WIDTH  = 4.0

-- ── State ─────────────────────────────────────────────────────────────────────

local boids        = {}
local boid_count   = BOID_COUNT_START
local time_alive   = 0.0
local spawn_needed = true

-- ── Helpers ───────────────────────────────────────────────────────────────────

local function random_range(lo, hi)
  return lo + math.random() * (hi - lo)
end

local function clamp_speed(v)
  local spd = v:length()
  if spd < 1e-6 then return v end
  local target = math.clamp(spd, SPEED_MIN * SPEED_SCALE, SPEED_MAX * SPEED_SCALE)
  return v * (target / spd)
end

local function heading(vel)
  return math.atan(vel.y, vel.x)
end

local function boid_vertices(pos, vel)
  local angle = heading(vel)
  local cos_a, sin_a = math.cos(angle), math.sin(angle)

  local nx = pos.x + cos_a * BOID_LENGTH
  local ny = pos.y + sin_a * BOID_LENGTH

  local lx = pos.x - cos_a * (BOID_LENGTH * 0.4) - sin_a * BOID_WIDTH
  local ly = pos.y - sin_a * (BOID_LENGTH * 0.4) + cos_a * BOID_WIDTH

  local rx = pos.x - cos_a * (BOID_LENGTH * 0.4) + sin_a * BOID_WIDTH
  local ry = pos.y - sin_a * (BOID_LENGTH * 0.4) - cos_a * BOID_WIDTH

  return nx, ny, lx, ly, rx, ry
end

-- ── Spawn ─────────────────────────────────────────────────────────────────────

local function spawn_boids()
  local W = Scene.Width
  local H = Scene.Height
  boids = {}

  for i = 1, boid_count do
    local angle = random_range(0, math.pi * 2)
    local speed = random_range(SPEED_MIN, SPEED_MAX)
    local hue   = i / boid_count

    local c = Color.Lerp(
      Color.Hex("#4fc3f7"),
      Color.Hex("#ef9a9a"),
      hue
    )

    boids[i] = {
      pos   = vec2(random_range(60, W - 60), random_range(60, H - 60)),
      vel   = vec2(math.cos(angle) * speed, math.sin(angle) * speed),
      color = c,
    }
  end

  spawn_needed = false
end

-- ── Steering ──────────────────────────────────────────────────────────────────

local function steer(i, mouse, repel)
  local b = boids[i]

  local sep            = vec2(0, 0)
  local align          = vec2(0, 0)
  local cohesion_pos   = vec2(0, 0)
  local sep_count      = 0
  local align_count    = 0
  local cohesion_count = 0

  for j = 1, #boids do
    if j ~= i then
      local other = boids[j]
      local d = b.pos:distance(other.pos)

      if d < RADIUS_SEPARATION and d > 0.001 then
        -- Inverse-distance weighted: closer neighbours push harder
        local away = (b.pos - other.pos) * (1.0 / d)
        sep = sep + away
        sep_count = sep_count + 1
      end

      if d < RADIUS_ALIGNMENT then
        align = align + other.vel
        align_count = align_count + 1
      end

      if d < RADIUS_COHESION then
        cohesion_pos = cohesion_pos + other.pos
        cohesion_count = cohesion_count + 1
      end
    end
  end

  local steering = vec2(0, 0)

  if sep_count > 0 then
    local avg_sep = sep * (1.0 / sep_count)
    steering = steering + avg_sep:normalize() * W_SEPARATION
  end

  if align_count > 0 then
    local avg_vel = align * (1.0 / align_count)
    steering = steering + avg_vel:normalize() * W_ALIGNMENT
  end

  if cohesion_count > 0 then
    local center    = cohesion_pos * (1.0 / cohesion_count)
    local to_center = center - b.pos
    local dist      = to_center:length()
    if dist > 0.001 then
      -- Scale by distance so far-away boids get pulled harder.
      -- Normalised by radius so the weight stays meaningful.
      local scaled = to_center:normalize() * (dist / RADIUS_COHESION)
      steering = steering + scaled * W_COHESION
    end
  end

  -- Mouse influence
  local to_mouse   = mouse - b.pos
  local mouse_dist = to_mouse:length()
  if mouse_dist < MOUSE_RADIUS and mouse_dist > 0.5 then
    local falloff = 1.0 - (mouse_dist / MOUSE_RADIUS)
    local dir     = to_mouse:normalize()
    if repel then
      steering = steering + (-dir) * (W_MOUSE_REPEL * falloff)
    else
      steering = steering + dir * (W_MOUSE_ATTRACT * falloff)
    end
  end

  return steering
end

-- ── Update ────────────────────────────────────────────────────────────────────

local function update(dt)
  local W     = Scene.Width
  local H     = Scene.Height
  local mpos  = Input.MousePosition()
  local mouse = vec2(mpos.x, mpos.y)
  local repel = Input.IsMouseDown(1)

  -- Compute all steerings before applying any, so every boid
  -- reacts to last frame's positions, not mid-update positions.
  local steerings = {}
  for i = 1, #boids do
    steerings[i] = steer(i, mouse, repel)
  end

  for i = 1, #boids do
    local b = boids[i]
    b.vel   = b.vel + steerings[i] * dt
    b.vel   = clamp_speed(b.vel)
    b.pos   = b.pos + b.vel * dt

    if b.pos.x < -BOID_LENGTH    then b.pos.x = W + BOID_LENGTH end
    if b.pos.x > W + BOID_LENGTH then b.pos.x = -BOID_LENGTH    end
    if b.pos.y < -BOID_LENGTH    then b.pos.y = H + BOID_LENGTH  end
    if b.pos.y > H + BOID_LENGTH then b.pos.y = -BOID_LENGTH     end
  end
end

-- ── Render ────────────────────────────────────────────────────────────────────

local function render()
  for i = 1, #boids do
    local b = boids[i]
    local nx, ny, lx, ly, rx, ry = boid_vertices(b.pos, b.vel)

    Draw.Line(lx, ly, nx, ny, b.color)
    Draw.Line(rx, ry, nx, ny, b.color)
    Draw.Line(lx, ly, rx, ry,
      Color.RGBA(b.color.r, b.color.g, b.color.b, 0.45))
  end

  -- Mouse cursor indicator
  local mpos  = Input.MousePosition()
  local repel = Input.IsMouseDown(1)
  local cursor_color = repel
    and Color.RGBA(1.0, 0.3, 0.3, 0.6)
    or  Color.RGBA(0.3, 1.0, 0.7, 0.6)

  Draw.Circle(mpos.x, mpos.y, MOUSE_RADIUS, cursor_color)
  Draw.FilledCircle(mpos.x, mpos.y, 4,
    Color.RGBA(cursor_color.r, cursor_color.g, cursor_color.b, 1.0))

  -- HUD
  local dim = Color.RGBA(1, 1, 1, 0.45)
  Draw.Text("BOIDS: " .. #boids,                       12, 12, 16, Color.RGBA(1,1,1,0.8))
  Draw.Text("UP/DOWN: add / remove boids",             12, 34, 12, dim)
  Draw.Text("LEFT/RIGHT: speed down / up",             12, 50, 12, dim)
  Draw.Text("MOUSE: attract   LMB: repel",             12, 66, 12, dim)
  Draw.Text(string.format("SPEED x%.2f", SPEED_SCALE), 12, 82, 12, dim)
end

-- ── Entry points ──────────────────────────────────────────────────────────────

function OnUpdate(dt)
  time_alive = time_alive + dt

  if Input.IsKeyJustPressed("Up") then
    boid_count   = math.min(boid_count + 20, BOID_COUNT_MAX)
    spawn_needed = true
  end
  if Input.IsKeyJustPressed("Down") then
    boid_count   = math.max(boid_count - 20, BOID_COUNT_MIN)
    spawn_needed = true
  end
  if Input.IsKeyJustPressed("Right") then
    SPEED_SCALE = math.min(SPEED_SCALE + 0.25, 3.0)
  end
  if Input.IsKeyJustPressed("Left") then
    SPEED_SCALE = math.max(SPEED_SCALE - 0.25, 0.25)
  end

  if spawn_needed then spawn_boids() end

  update(dt)
end

function OnRender()
  render()
end
