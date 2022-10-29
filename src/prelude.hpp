#pragma once

constexpr const auto prelude = R"(
func pow base exp
    let out = base
    if exp == 0
        return out
    end
    while exp > 1
        out *= base
        exp -= 1
    end
    return out
end

func mod numerator denominator
    let tmp = 0
    tmp = numerator / denominator
    denominator *= tmp
    tmp = numerator - denominator
    return tmp
end

func prime n
    if n == 0
        return 0
    end
    if n == 1
        return 0
    end
    let check = 2
    while check < n
        mod n check
        if _ == 0
            return 0
        end
        check += 1
    end
    return 1
end

func gcd x y
    while x != y
        if x > y
            x -= y
        end
        if x <= y
            y -= x
        end
    end
    return y
end
)";
