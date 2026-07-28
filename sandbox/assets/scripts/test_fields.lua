Name = "TestFields"
--------------------------------------------------
-- Numbers
--------------------------------------------------

-- Default:
-- Should display as DragFloat
speed = 50


-- Range only:
-- Should display as DragFloat with min/max limits
Range(0, 10)
jumpCount = 2


-- Range + step:
-- Should display as DragFloat
-- Drag increments should be 0.5
Range(0, 100, 0.5)
health = 75


-- Large range:
-- Tests that drag precision still feels good
Range(-1000, 1000, 10)
worldPosition = 250


--------------------------------------------------
-- Slider
--------------------------------------------------

-- Explicit slider widget
-- Requires Range
Widget("slider")
Range(0, 1, 0.01)
volume = 0.8


-- Percentage slider
Widget("slider")
Range(0, 100, 1)
completion = 35


--------------------------------------------------
-- Invalid combinations
--------------------------------------------------

-- Should show an error:
-- "Widget(color) requires a table"
Widget("color")
invalidColor = 0.5


--------------------------------------------------
-- Colors
--------------------------------------------------

-- Should display ColorEdit3
Widget("color")
tint = {0.2, 0.8, 0.3}


-- Another color test
Widget("color")
ambientColor = {1.0, 0.5, 0.1}


--------------------------------------------------
-- Strings
--------------------------------------------------

displayName = "Player"

Tooltip("Name shown in the inspector")
characterName = "Axiom"


--------------------------------------------------
-- Booleans
--------------------------------------------------

isEnabled = true

Tooltip("Whether this object receives updates")
active = false


--------------------------------------------------
-- Private fields
--------------------------------------------------

local privateCounter = 0
local internalState = "hidden"


--------------------------------------------------
-- Lifecycle
--------------------------------------------------

function OnStart()
    print(Name .. " started")
    print("speed:", speed)
    print("jumpCount:", jumpCount)
    print("health:", health)
end


function OnUpdate()
    privateCounter = privateCounter + 1

    if privateCounter % 60 == 0 then
        print(
            Name ..
            " update - speed=" ..
            speed ..
            " health=" ..
            health
        )
    end
end


function OnDestroy()
    print(Name .. " destroyed")
end
