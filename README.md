# VM RISK-XVII

## Description

A custom virtual machine named vm_RISKXVII with heap banks and a unique RISK-XVII instruction set architecture (based on RV32I), allowing execution of binary programs compiled for RV32I.
33 instructions are implemented, covering arithmetic/logic operations, memory access, and program flow operations, ensuring Turing completeness.
Virtual routines for I/O operations, including console write/read functions and memory allocation/freeing are available.
A total of 128 memory banks for dynamic allocation of data.

## Getting Started

### Executing program

* Once program is run, it will accept a single command line argument being the path to the file containing your RISK-XVII assembly code. The virtual machine will then start running the assembly code.
