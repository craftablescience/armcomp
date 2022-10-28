# armcomp
A basic language that compiles to ARM assembly so I can write less assembly!

## requirements
Python 3.6 or greater must be installed to use this application.
The code may rely on C++20 features, I haven't checked. You will need at least a C++17 capable compiler.

## acknowledgements
Without ARMSim there would have been no way to run the generated assembly program. To the people that worked on that, thank you ♥️

## sample code
This example code:
```fs
let x = 12
let y = x
x = y + 8
y = x * x

if x == 20
    println "x is currently 20!"
end

while y > x
    x += 100
    println "BUNP"
end

println "that's all folks!"
exit x
```
Turns into this ARM assembly code:
```arm
.text
.global _start
_start:
    mov x10, #12
    mov x11, x10
    add x9, x11, #8
    mov x10, x9
    mul x9, x10, x10
    mov x11, x9
    cmp x10, #20
    bne ._i0
    mov x0, #1
    ldr x1, =_s0
    ldr x2, =_s0_len
    mov x8, 0x40
    svc 0
._i0:
._w1:
    cmp x11, x10
    ble ._w2
    add x10, x10, #100
    mov x0, #1
    ldr x1, =_s1
    ldr x2, =_s1_len
    mov x8, 0x40
    svc 0
    b ._w1
._w2:
    mov x0, #1
    ldr x1, =_s2
    ldr x2, =_s2_len
    mov x8, 0x40
    svc 0
    mov x0, x10
    mov x8, #93
    svc 0
.data
_s0: .asciz "x is currently 20!\n"
_s0_len = .-_s0
_s1: .asciz "BUNP\n"
_s1_len = .-_s1
_s2: .asciz "that's all folks!\n"
_s2_len = .-_s2
```
