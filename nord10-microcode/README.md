## Nord-10 microcode manipulation programs
The Nord-10 microcode is 32 bits wide and hence handled as lists of 32-bit words.
It may be either 1k (standard) or 4k (CE extension).

The files are:
### dismac
- A program that reads a hex file (given as argument) and outputs micmac assembler

### micmac.awk
- A program that reads a micmac assembler file and outputs a hex file

### epgtest
- A program that takes an Nord-10 opcode and outputs its entry in the microcode

### timing
- A simple test program that prints out the clocking pulses on Nord-10

### uc-opc
- Contains mnemonics, opcodes and entry points for all standard Nord-10 instructions

### prom.hex/prom4k.hex
- Hex files of the 1k/4k prom dumps.

### nd10uc
- Microcode emulator for the Nord-10. Not especially well implemented, but somewhat works.
