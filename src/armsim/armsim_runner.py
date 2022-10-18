import armsim
import sys

with open(sys.argv[1], 'r') as asm:
    armsim.parse(asm.readlines())
armsim.run()

for i in range(10, 29):
    regval = armsim.reg[f'x{i}']
    if regval == 0:
        continue
    print(f'X{i}: {regval}')

exit(armsim.reg['x0'])
