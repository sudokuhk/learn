function counter()
    local count = 0
    return function()
    count = count + 1
    return count
    end
end

print counter()
