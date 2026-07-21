local counter = 0
print("script is running")
function OnUpdate(self, dt)
    counter = counter + dt
    print(string.format("alive for %.2f seconds", counter))
end
