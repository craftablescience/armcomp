# armcomp
A basic language that compiles to ARM assembly so I can write less assembly!

## requirements
Python 3.6 or greater must be installed to use this application.
The code may rely on C++20 features, I haven't checked. You will need at least a C++17 capable compiler.

## acknowledgements
Without ARMSim there would have been no way to run the generated assembly program. To the people that worked on that, thank you ♥️

## commands
- `if` - Execute the inner code if the condition is true
- `while` - Run the inner code until the condition is false
- `func` - Create a new function with optional arguments
- `return` - Optionally return a value from a function, or exit the function early
- `asm` - Insert raw assembly. Variables wrapped in `${}` are converted to the register they represent
- `end` - Ends a block statement (`if`, `while`, `func`, `asm`)
- `let` - Create a new variable
- `label` - Insert a label at this position
- `goto` - Jump to a given label
- `print` - Print the given string
- `println` - Print the given string, with a newline inserted at the end (for convenience)
- `exit` - Exit the program, with an optional return value
- `<variable name here>` - Modify a variable
- `<function name here>` - Execute a function that has been defined. Anything returned is stored in the special `_` variable

## sample code
See also: [the "standard library"](https://github.com/craftablescience/armcomp/blob/main/src/prelude.hpp)

This example code:
```fs
let x = 12
let y = x
x = y + 8
y = x * x

func important_check in
    if in == 20
        println "x is currently 20!"
    end
end

important_check x

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
	mov x11, #12
	mov x12, x11
	add x9, x12, #8
	mov x11, x9
	mul x9, x11, x11
	mov x12, x9
	mov x0, x11
	str x11, [sp, #-0x10]!
	str x12, [sp, #-0x10]!
	bl important_check
	ldr x12, [sp], #0x10
	ldr x11, [sp], #0x10
._while1:
	cmp x12, x11
	ble ._while2
	add x11, x11, #100
	mov x0, #1
	ldr x1, =_str1
	ldr x2, =_str1_len
	mov x8, 0x40
	svc 0
	b ._while1
._while2:
	mov x0, #1
	ldr x1, =_str2
	ldr x2, =_str2_len
	mov x8, 0x40
	svc 0
	mov x0, x11
	mov x8, #93
	svc 0
	b ._proc_end
important_check:
	str lr, [sp, #-0x10]!
	mov x11, x0
	cmp x11, #20
	bne ._if0
	mov x0, #1
	ldr x1, =_str0
	ldr x2, =_str0_len
	mov x8, 0x40
	svc 0
._if0:
	ldr lr, [sp], #0x10
	ret
._proc_end:
.data
_str0: .asciz "x is currently 20!\n"
_str0_len = .-_str0
_str1: .asciz "BUNP\n"
_str1_len = .-_str1
_str2: .asciz "that's all folks!\n"
_str2_len = .-_str2
```
