Name = "player_sprite"
local texture
local transform

function OnStart()
    texture = Assets.LoadTexture("sprites/ship.png")
    transform = self:GetTransform()
    transform.x = transform.x + 64
    transform.y = transform.y + 64
end

function OnRender()
    DrawSprite(texture, transform.x, transform.y, 0, 0, transform.rotation)
end
