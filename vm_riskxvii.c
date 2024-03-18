#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "structs_enums.h"
#include "readfile.h"
#include "parse.h"
#include "store_load_helper.h"
#include "vir_routine.h"
#include "heap.h"


// FILE HANDLING FUNCTIONS (readfile.h)
void read_file_into_mem(struct VM *vm, char *filename) {
    FILE *file = fopen(filename, "rb");
    if(file == NULL) {
        printf("File does not exist\n");
    }
    // read instruction mem byte by byte
    int read_num = fread(vm->inst_mem, 1, INST_MEM_SIZE, file);

    int read_num2 = fread(vm->data_mem, 1, DATA_MEM_SIZE, file);

    if(read_num != DATA_MEM_SIZE || read_num2 != DATA_MEM_SIZE) {
        printf("fread error\n");
    }
    fclose(file);
}


// get full instruction lines
void get_inst_lines(struct VM *vm, char *filename) {
    FILE *file = fopen(filename, "rb");
    if(file == NULL) {
        printf("File does not exist\n");
    }
    int read_num = fread(vm->inst_lines, 4, INST_MEM_SIZE/4, file);

    if(read_num != INST_MEM_SIZE/4) {
        printf("fread error\n");
    }
    fclose(file);
}


// PARSING FUNCTIONS (parse.h)
uint32_t extract_bits(uint32_t instruction, int start_index, int end_index) {
    uint32_t bits = (instruction << (31-start_index)) >> ((31-start_index)+end_index);
    return bits;
}

uint8_t extract_opcode(uint32_t instruction) {
    // bit shift to get opcode of instruction given
    // left shift to get least sig 7 bits then right shift to get right power
    uint8_t opcode = extract_bits(instruction, 6, 0) & 0xFF;
    // printf("opcode: %x\n", opcode);
    return opcode;
}

uint8_t extract_rd(uint32_t instruction) {
    // left shift by 31 - start index
    // right shift by (31 - start index) + end index
    uint8_t rd = extract_bits(instruction, 11, 7) & 0xFF;
    return rd;
}

uint8_t extract_func3(uint32_t instruction) {
    uint8_t func3 = extract_bits(instruction, 14, 12) & 0xFF;
    return func3;
}

uint8_t extract_rs1(uint32_t instruction) {
    uint8_t rs1 = extract_bits(instruction, 19, 15) & 0xFF;
    return rs1;
}

uint8_t extract_rs2(uint32_t instruction) {
    uint8_t rs2 = extract_bits(instruction, 24, 20) & 0xFF;
    return rs2;
}

uint8_t extract_func7(uint32_t instruction) {
    uint8_t func7 = extract_bits(instruction, 31, 25) & 0xFF;
    return func7;
}

int is_negative(uint32_t imm, int length) {
    // get msb
    // bitshift
    uint32_t msb = extract_bits(imm, length, length);
    // check if msb is 1
    if((msb & 0b1) != 1) {
        return 0;
    }
    return 1;
}

uint32_t get_2s_comp(uint32_t imm, int length) {
    // get 2s complement value
    // convert 1s to 0s and vice versa, and add 1
    uint32_t new_imm = (~imm) + 0b1; 
    // bitshift to remove effect of flipping 0 padding
    new_imm = (new_imm << (31-length)) >> (31-length);
    return new_imm;
} 


int convert_2s_comp(uint32_t imm, int length) {
    if(is_negative(imm, length) == 1) {
        int signed_imm = -get_2s_comp(imm, length);
        return signed_imm;
    }
    return imm;
}

enum TYPE inst_type(uint8_t opcode) {
    if((opcode ^ TYPE_I) == 0) {
        return TYPE_I;
    }
    if((opcode ^ TYPE_I_JMP) == 0) {
        return TYPE_I_JMP;
    }
    if((opcode ^ TYPE_I_LOAD) == 0) {
        return TYPE_I_LOAD;
    }
    if((opcode ^ TYPE_R) == 0) {
        return TYPE_R;
    }
    if((opcode ^ TYPE_S) == 0) {
        return TYPE_S;
    }
    if((opcode ^ TYPE_SB) == 0) {
        return TYPE_SB;
    }
    if((opcode ^ TYPE_U) == 0) {
        return TYPE_U;
    }
    if((opcode ^ TYPE_UJ) == 0) {
        return TYPE_UJ;
    }
    return TYPE_INVALID;
}


// ARITHMETIC AND LOGIC OPERATIONS
void add(struct VM *vm, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    vm->registers[rd] = vm->registers[rs1] + vm->registers[rs2];
}

void addi(struct VM *vm, uint8_t rd, uint8_t rs1, uint32_t imm) {
    vm->registers[rd] = vm->registers[rs1] + imm;
}

void sub(struct VM *vm, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    vm->registers[rd] = vm->registers[rs1] - vm->registers[rs2];
}

void lui(struct VM *vm, uint8_t rd, uint32_t imm) {
    vm->registers[rd] = imm;
}

void xor(struct VM *vm, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    vm->registers[rd] = vm->registers[rs1] ^ vm->registers[rs2];
}

void xori(struct VM *vm, uint8_t rd, uint8_t rs1, uint32_t imm) {
    vm->registers[rd] = vm->registers[rs1] ^ imm;
}

void or(struct VM *vm, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    vm->registers[rd] = vm->registers[rs1] | vm->registers[rs2];
}

void ori(struct VM *vm, uint8_t rd, uint8_t rs1, uint32_t imm) {
    vm->registers[rd] = vm->registers[rs1] | imm;
}

void and(struct VM *vm, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    vm->registers[rd] = vm->registers[rs1] & vm->registers[rs2];
}

void andi(struct VM *vm, uint8_t rd, uint8_t rs1, uint32_t imm) {
    vm->registers[rd] = vm->registers[rs1] & imm;
}

// STORE AND LOAD HELPERS (store_load_helper.h)
// store mem bytes
void store_mem_bytes(struct VM *vm, uint8_t rs1, int imm, uint32_t val, int num_bytes) {
    uint32_t addr = vm->registers[rs1] + imm - 0x0400;
    uint32_t heap_addr = ((addr+0x0400)-0xb700)/64;
    int32_t val2 = (int32_t) val;
    for(int i = 0 ; i < num_bytes ; i++) {
        if (addr <= 0x0400) {
            vm->data_mem[addr+i] = extract_bits(val2, (8*(i+1))-1, 8*i);
        }
        else {
            // store in bank
            vm->heap[heap_addr].heap_data[i] = extract_bits(val2, (8*(i+1))-1, 8*i);
        }
    }
}

int32_t sext(uint32_t val, int num_bits) {
    // >> is logical shift
    int32_t sign_ext = (int32_t) (val << (32-num_bits)) >> (32-num_bits);
    return sign_ext;
}

// get register value (load instructions)
uint32_t get_rs_val(struct VM *vm, uint8_t rs1, int imm) {
    uint32_t addr = vm->registers[rs1] + imm;
    if(addr >= 0x0400 && addr <= 0x7ff) {
        return vm->data_mem[addr - 0x0400];
    }
    else if (addr <= 0x3ff) {
        return vm->inst_mem[addr];
    }
    else if(addr >= 0xb700) {
        uint32_t starting_index = ((addr-0xb700)-((addr-0xb700)%64))/64;
        return vm->heap[starting_index].heap_data[(addr-0xb700)%64];
    }
    return 0;
}


// get memory data according to number of bytes
uint32_t get_mem_bytes(struct VM *vm, uint8_t rs1, int imm, int num_bytes) {
    uint32_t full_data = 0;
    for(int i = 0 ; i < num_bytes ; i++) {
        // shift each byte to get correct value
        // uint32_t new_byte = get_rs_val(vm, rs1, (imm+i)) << ((num_bytes-i-1)*8);
        uint32_t new_byte = get_rs_val(vm, rs1, (imm+i)) << (i*8);
        full_data = full_data | new_byte;
    }
    return full_data;
}


// VIRTUAL ROUTINE FUNCTIONS (vir_routine.h)
// check if store command vr
uint32_t is_store_vr(char *inst_name, uint8_t rs1, int imm, struct VM *vm) {
    if(strcmp(inst_name, "sb") == 0 || strcmp(inst_name, "sh") == 0 || strcmp(inst_name, "sw") == 0) {
        uint32_t addr = vm->registers[rs1] + imm;
        // printf("index in func: %x\n", addr);
        // check if address is virtual routine
        if(addr == VIR_W_CHAR || addr == VIR_W_INT || addr == VIR_W_UINT || 
        addr == VIR_HALT || addr == VIR_DUMP_PC || addr == VIR_DUMP_REG || addr == VIR_DUMP_MEM) {
            return addr;
        }
    }
    return 0;
}

// check if load command vr
uint32_t is_load_vr(char *inst_name, uint8_t rs1, int imm, struct VM *vm) {
    if(strcmp(inst_name, "lb") == 0 || strcmp(inst_name, "lh") == 0 || strcmp(inst_name, "lw") == 0 || strcmp(inst_name, "lbu") == 0 || strcmp(inst_name, "lhu") == 0) {
        uint32_t addr = vm->registers[rs1] + imm;
        if(addr == VIR_R_CHAR || addr == VIR_R_INT) {
            return addr;
        }
    }
    return 0;
}


int get_num_bits(char *inst_name) {
    if(strcmp("sb", inst_name)==0 || strcmp("lb", inst_name)==0 || strcmp("lbu", inst_name)==0) {
        return 8;
    }
    if(strcmp("sh", inst_name)==0 || strcmp("lh", inst_name)==0 || strcmp("lhu", inst_name)==0) {
        return 16;
    }
    if(strcmp("sw", inst_name)==0 || strcmp("lw", inst_name)==0 || strcmp("lwu", inst_name)==0) {
        return 32;
    }
    return 0;
}

void w_char(uint32_t value, char *inst_name) {
    int num_bits = get_num_bits(inst_name);
    printf("%c", extract_bits(value, num_bits-1, 0));
}

void w_int(uint32_t value, char *inst_name) {
    int num_bits = get_num_bits(inst_name);
    printf("%d", extract_bits(value, num_bits-1, 0));
}

void w_uint(uint32_t value, char *inst_name) {
    int num_bits = get_num_bits(inst_name);
    printf("%x", extract_bits(value, num_bits-1, 0));
}

void halt() {
    printf("CPU Halt Requested\n");
    exit(0);
}

void dump_PC(uint16_t PC) {
    printf("0x%04x", PC);
}

void dump_reg(struct VM *vm) {
    printf("PC = 0x%08x;\n", vm->PC);
    for(int i = 0 ; i < 32 ; i++) {
        printf("R[%d] = 0x%08x;\n", i, vm->registers[i]);
    }
}

void dump_mem(uint32_t value) {
    printf("%08x", value);
}

uint32_t r_char() {
    uint32_t char_code;
    scanf("%lc", &char_code);
    return char_code;
}

int32_t r_int() {
    int32_t scanned_int;
    scanf("%d", &scanned_int);
    return scanned_int;
} 

int exe_store_vr(struct INST *inst, struct VM *vm) {
    if(inst->type == TYPE_S && is_store_vr(inst->name, inst->type_info.S.rs1, inst->type_info.S.imm_signed, vm) != 0) {
        uint32_t addr = is_store_vr(inst->name, inst->type_info.S.rs1, inst->type_info.S.imm_signed, vm);
        switch(addr) {
            case VIR_HALT:
                halt();
                break;
            case VIR_W_CHAR:
                w_char(vm->registers[inst->type_info.S.rs2], inst->name);
                break;
            case VIR_W_INT:
                w_int(vm->registers[inst->type_info.S.rs2], inst->name);
                break;
            case VIR_W_UINT:
                w_uint(vm->registers[inst->type_info.S.rs2], inst->name);
                break;
            case VIR_DUMP_MEM:
                // get M[v] with v being R[rs2] then offset index
                dump_mem(vm->data_mem[vm->registers[inst->type_info.S.rs2] - 0x0400]);
                break;
            case VIR_DUMP_PC:
                dump_PC(vm->PC);
                break;
            case VIR_DUMP_REG:
                dump_reg(vm);
                break;

        }
        return 1;
    } 
    else {
        return 0;
    }
}

// HEAP BANK FUNCTIONS
uint32_t is_heap(char *inst_name, uint8_t rs1, int imm, struct VM *vm) {
    if(strcmp(inst_name, "sb") == 0 || strcmp(inst_name, "sh") == 0 || strcmp(inst_name, "sw") == 0) {
        uint32_t addr = vm->registers[rs1] + imm;
        // check if address is virtual routine
        if(addr == HEAP_MALLOC || addr == HEAP_FREE) {
            return addr;
        }
    }
    return 0;
}

uint32_t malloc_heap(struct VM *vm, uint32_t num_bytes) {
    // invalid index 65
    uint32_t start_index = 65;
    int num_banks = ((num_bytes-(num_bytes%64))/64 )+1;
    for(int i = 0 ; i < HEAP_BANK_NUM-num_banks ; i++) {
        for(int j = 0; j < num_banks ; j++) {
            if(vm->heap[i+j].bytes_allocated != 0) {
                break;
            }
            else if(j == (num_banks-1)) {
                start_index = i;
            }
        }
        if(start_index != 65) {
            // allocate the banks
            for(int k = start_index ; k < (num_banks+start_index) ; k++) {
                if((num_bytes%64 !=0) && (k == (num_banks+start_index-1))) {
                    vm->heap[k].bytes_allocated = num_bytes%64;
                }
                vm->heap[k].bytes_allocated = 64;
            }
            break;
        }
    }
    return start_index;
}

int exe_heap(struct INST *inst, struct VM *vm) {
    uint32_t addr = is_heap(inst->name, inst->type_info.S.rs1, inst->type_info.S.imm_signed, vm);
    if(addr == 0) {
        return 0;
    } 
    if(addr == HEAP_MALLOC){
        uint32_t start_index = malloc_heap(vm, vm->registers[inst->type_info.S.rs2]);
        if(start_index == 65) {
            vm->registers[28] = 0;
        }
        // get mapped address
        uint32_t ptr = 0xb700 + (start_index*64);
        // store mapped addr
        vm->registers[28] = ptr;
        return 1;
        
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // char *filename;
    // // get filename
    // // too many or too few arguments
    if(argc != 2) {
        printf("Wrong number of arguments\n");
        exit(1);
    }

    struct VM vm;
    read_file_into_mem(&vm, argv[1]);
    get_inst_lines(&vm, argv[1]);


    //initialise PC
    vm.PC = 0x0000;
    vm.PC_lines = 0x0000;

    // initialise all registers to 0
    for(int i = 0 ; i < 32 ; i++) {
        vm.registers[i] = 0;
    }

    while(vm.PC <= 0x3ff) {
        // initialise register 0
        vm.registers[0] = 0;

        // initialise an INST to capture all info 
        struct INST inst;

        // get index
        vm.PC_lines = vm.PC / 4;

        inst.line = vm.inst_lines[vm.PC_lines];
        inst.opcode = extract_opcode(inst.line);
        inst.type = inst_type(inst.opcode);


        // differentiate by type
        // assign values for each attribute
        switch(inst.type) {
            // type R
            case TYPE_R:
                inst.type_info.R.rd = extract_rd(inst.line);
                inst.type_info.R.func3 = extract_func3(inst.line);
                inst.type_info.R.rs1 = extract_rs1(inst.line);
                inst.type_info.R.rs2 = extract_rs2(inst.line);
                inst.type_info.R.func7 = extract_func7(inst.line);

                // find specific function
                // TODO check for func7 (if incorrect error handle)
                switch(inst.type_info.R.func3) {
                    case ADD_FUNC3:
                        if((inst.type_info.R.func7 ^ ADD_FUNC7) == 0) {
                            inst.name = "add";
                        }
                        else if((inst.type_info.R.func7 ^ SUB_FUNC7) == 0) {
                            inst.name = "sub";
                        }
                        break;
                    case XOR_FUNC3:
                        inst.name = "xor";
                        break;
                    case OR_FUNC3:
                        inst.name = "or";
                        break;
                    case AND_FUNC3:
                        inst.name = "add";
                        break;
                    case SLL_FUNC3:
                        inst.name = "sll";
                        break;
                    case SRL_FUNC3:
                        if((inst.type_info.R.func7 ^ SRL_FUNC7) == 0){
                            inst.name = "srl";
                        }
                        else if((inst.type_info.R.func7 ^ SRA_FUNC7) == 0) {
                            inst.name = "sra";
                        }
                        break;
                    case SLT_FUNC3:
                        inst.name = "slt";
                        break;
                    case SLTU_FUNC3:
                        inst.name = "sltu";
                        break;
                }
                break;
            // type I
            case TYPE_I_JMP:
                inst.type_info.I.rd = extract_rd(inst.line);
                inst.type_info.I.func3 = extract_func3(inst.line);
                inst.type_info.I.rs1 = extract_rs1(inst.line);
                inst.type_info.I.imm = extract_bits(inst.line, 31, 20);
                inst.type_info.I.imm_signed = sext(inst.type_info.I.imm, 12);

                switch(inst.type_info.I.func3) {
                    case JALR_FUNC3:
                        inst.name = "jalr";
                        break;
                }
                break;
            case TYPE_I_LOAD:
                inst.type_info.I.rd = extract_rd(inst.line);
                inst.type_info.I.func3 = extract_func3(inst.line);
                inst.type_info.I.rs1 = extract_rs1(inst.line);
                inst.type_info.I.imm = extract_bits(inst.line, 31, 20);
                inst.type_info.I.imm_signed = sext(inst.type_info.I.imm, 12);

                switch(inst.type_info.I.func3) {
                    case LB_FUNC3:
                        inst.name = "lb";
                        break;
                    case LH_FUNC3:
                        inst.name = "lh";
                        break;
                    case LW_FUNC3:
                        inst.name = "lw";
                        break;
                    case LBU_FUNC3:
                        inst.name = "lbu";
                        break;
                    case LHU_FUNC3:
                        inst.name = "lhu";
                        break;
                }
                break;
            case TYPE_I:
                inst.type_info.I.rd = extract_rd(inst.line);
                inst.type_info.I.func3 = extract_func3(inst.line);
                inst.type_info.I.rs1 = extract_rs1(inst.line);
                inst.type_info.I.imm = extract_bits(inst.line, 31, 20);
                inst.type_info.I.imm_signed = sext(inst.type_info.I.imm, 12);

                switch(inst.type_info.I.func3) {
                    case ADDI_FUNC3:
                        inst.name = "addi";
                        break;
                    case XORI_FUNC3:
                        inst.name = "xori";
                        break;
                    case ORI_FUNC3:
                        inst.name = "ori";
                        break;
                    case ANDI_FUNC3:
                        inst.name = "andi";
                        break;
                    case SLTI_FUNC3:
                        inst.name = "slti";
                        break;
                    case SLTIU_FUNC3:
                        inst.name = "sltiu";
                        break;
                }
                break;
            // type S
            case TYPE_S:
                inst.type_info.S.imm1 = extract_bits(inst.line, 11, 7);
                inst.type_info.S.func3 = extract_func3(inst.line);
                inst.type_info.S.rs1 = extract_rs1(inst.line);
                inst.type_info.S.rs2 = extract_rs2(inst.line);
                inst.type_info.S.imm2 = extract_bits(inst.line, 31, 25) << 5;   // offset by 5 bits (bit 25 mapped to bit 5)
                inst.type_info.S.imm = inst.type_info.S.imm1 | inst.type_info.S.imm2;
                inst.type_info.S.imm_signed = sext(inst.type_info.S.imm, 12);

                switch(inst.type_info.S.func3) {
                    case SB_FUNC3:
                        inst.name = "sb";
                        break;
                    case SH_FUNC3:
                        inst.name = "sh";
                        break;
                    case SW_FUNC3:
                        inst.name = "sw";
                        break;
                }
                break;
            // type SB
            case TYPE_SB:
                inst.type_info.SB.func3 = extract_func3(inst.line);
                inst.type_info.SB.imm1 = (extract_bits(inst.line, 7, 7) << 11) | (extract_bits(inst.line, 11, 8) << 1);    // bit 7 offset by 11 and bits 8-11 offset by 1
                inst.type_info.SB.rs1 = extract_rs1(inst.line);
                inst.type_info.SB.rs2 = extract_rs2(inst.line);
                inst.type_info.SB.imm2 = (extract_bits(inst.line, 30, 25) << 5) | (extract_bits(inst.line, 31, 31) << 12);
                inst.type_info.S.imm = inst.type_info.SB.imm1 | inst.type_info.SB.imm2;
                inst.type_info.S.imm_signed = sext(inst.type_info.S.imm, 13);

                switch(inst.type_info.SB.func3) {
                    case BEQ_FUNC3:
                        inst.name = "beq";
                        break;
                    case BNE_FUNC3:
                        inst.name = "bne";
                        break;
                    case BLT_FUNC3:
                        inst.name = "blt";
                        break;
                    case BLTU_FUNC3:
                        inst.name = "bltu";
                        break;
                    case BGE_FUNC3:
                        inst.name = "bge";
                        break;
                    case BGEU_FUNC3:
                        inst.name = "bgeu";
                        break;
                }
                break;
            // type U
            case TYPE_U:
                inst.type_info.U.rd = extract_rd(inst.line);
                inst.type_info.U.imm = (extract_bits(inst.line, 31, 12) << 12);
                inst.type_info.U.imm_signed = sext(inst.type_info.U.imm, 32);
                inst.name = "lui";
                break;
            // type UJ
            case TYPE_UJ:
                inst.type_info.UJ.rd = extract_rd(inst.line);
                inst.type_info.UJ.imm = (extract_bits(inst.line, 19, 12) << 12) | (extract_bits(inst.line, 20, 20) << 11);     //extract bit 12:19 and bit 11
                inst.type_info.UJ.imm = inst.type_info.UJ.imm | (extract_bits(inst.line, 30, 21) << 1) | (extract_bits(inst.line, 31, 31) << 20);
                inst.type_info.UJ.imm_signed = sext(inst.type_info.UJ.imm, 21);
                inst.name = "jal";
                break;
            // type invalid
            case TYPE_INVALID:
                printf("Instruction Not Implemented: 0x%08x\n", inst.line);
                dump_reg(&vm);
                exit(1);

        }

        //VIRTUAL ROUTINE CHECK
        int load_vr = 0;
        // check halt
        if (exe_store_vr(&inst, &vm) == 1){
            // increment PC
            vm.PC += 4;
            continue;
        }
        // check load
        else if(strcmp("lb", inst.name) == 0 || strcmp("lh", inst.name) == 0 || strcmp("lw", inst.name) == 0 ||strcmp("lbu", inst.name) == 0 || strcmp("lhu", inst.name) == 0) {
            if(is_load_vr(inst.name, inst.type_info.I.rs1, inst.type_info.I.imm, &vm) != 0) {
                load_vr = 1;
            }
        }
        // check malloc
        else if(exe_heap(&inst, &vm) == 1) {
            vm.PC += 4;
            continue;
        }


        // arithmetic and logic operations
        if(strcmp("add", inst.name) == 0) {
            add(&vm, inst.type_info.R.rd, inst.type_info.R.rs1, inst.type_info.R.rs2);
        }
        if(strcmp("addi", inst.name) == 0) {
            addi(&vm, inst.type_info.I.rd, inst.type_info.I.rs1, inst.type_info.I.imm_signed);
        }
        if(strcmp("sub", inst.name) == 0) {
            sub(&vm, inst.type_info.R.rd, inst.type_info.R.rs1, inst.type_info.R.rs2);
        }
        if(strcmp("lui", inst.name) == 0) {
            lui(&vm, inst.type_info.U.rd, inst.type_info.U.imm_signed);
        }
        if(strcmp("xor", inst.name) == 0) {
            xor(&vm, inst.type_info.R.rd, inst.type_info.R.rs1, inst.type_info.R.rs2);
        }
        if(strcmp("xori", inst.name) == 0) {
            xori(&vm, inst.type_info.I.rd, inst.type_info.I.rs1, inst.type_info.I.imm_signed);
        }
        if(strcmp("or", inst.name) == 0) {
            or(&vm, inst.type_info.R.rd, inst.type_info.R.rs1, inst.type_info.R.rs2);
        }
        if(strcmp("ori", inst.name) == 0) {
            ori(&vm, inst.type_info.I.rd, inst.type_info.I.rs1, inst.type_info.I.imm_signed);
        }
        if(strcmp("and", inst.name) ==  0) {
            and(&vm, inst.type_info.R.rd, inst.type_info.R.rs1, inst.type_info.R.rs2);
        }
        if(strcmp("andi", inst.name) == 0) {
            andi(&vm, inst.type_info.I.rd, inst.type_info.I.rs1, inst.type_info.I.imm_signed);
        }
        if(strcmp("sll", inst.name) == 0) {
            vm.registers[inst.type_info.R.rd] = vm.registers[inst.type_info.R.rs1] << vm.registers[inst.type_info.R.rs2];
        }
        if(strcmp("srl", inst.name) == 0) {
            vm.registers[inst.type_info.R.rd] = vm.registers[inst.type_info.R.rs1] >> vm.registers[inst.type_info.R.rs2];
        }
        if(strcmp("sra", inst.name) == 0) {
            // get the shifted num
            int shift_amount = vm.registers[inst.type_info.R.rs2];
            uint32_t shifted = vm.registers[inst.type_info.R.rs1] >> vm.registers[shift_amount];
            uint32_t out_of_frame = vm.registers[inst.type_info.R.rs1] << (32-shift_amount);
            vm.registers[inst.type_info.R.rd] = shifted | out_of_frame;
        }
        
        // Memory access operations
        if(strcmp("lb", inst.name) == 0) {
            //8 bit
            if(load_vr == 1) {
                if (is_load_vr(inst.name, inst.type_info.I.rs1, inst.type_info.I.imm, &vm) == VIR_R_CHAR) {
                    vm.registers[inst.type_info.I.rd] = (int32_t) sext(extract_bits(r_char(), 7, 0), 8);
                }
                else {
                    vm.registers[inst.type_info.I.rd] = (int32_t) sext(extract_bits(r_int(), 7, 0), 8);
                }
                load_vr = 0;
            }
            else {
                // get first byte then get sign extended value
                int32_t sign_ext = sext(get_rs_val(&vm, inst.type_info.I.rs1, inst.type_info.I.imm_signed), 8);
                vm.registers[inst.type_info.I.rd] = (int32_t) sign_ext;
            }
        }
        if(strcmp("lh", inst.name) == 0) {
            // 16 bit
            if(load_vr == 1) {
                if (is_load_vr(inst.name, inst.type_info.I.rs1, inst.type_info.I.imm, &vm) == VIR_R_CHAR) {
                    vm.registers[inst.type_info.I.rd] = (int32_t) sext(extract_bits(r_char(), 15, 0), 16);
                }
                else {
                    vm.registers[inst.type_info.I.rd] = (int32_t) sext(extract_bits(r_int(), 15, 0), 16);
                }
                load_vr = 0;
            }
            else {
                int32_t sign_ext = extract_bits(get_mem_bytes(&vm, inst.type_info.I.rs1, inst.type_info.I.imm_signed, 2), 15, 0);
                sign_ext = sext(sign_ext, 16);
                vm.registers[inst.type_info.I.rd] = (int32_t) sign_ext;
            }
        }
        if(strcmp("lw", inst.name) == 0) {
            // 32 bit
            if(load_vr == 1) {
                if (is_load_vr(inst.name, inst.type_info.I.rs1, inst.type_info.I.imm, &vm) == VIR_R_CHAR) {
                    vm.registers[inst.type_info.I.rd] = (int32_t) r_char();
                }
                else {
                    vm.registers[inst.type_info.I.rd] = (int32_t) r_int();
                }
                load_vr = 0;
            }
            else {
                vm.registers[inst.type_info.I.rd] = (int32_t) get_mem_bytes(&vm, inst.type_info.I.rs1, inst.type_info.I.imm_signed, 4);
            }        
        }
        if(strcmp("lbu", inst.name) == 0) {
            // unsigned 8 bit
            if(load_vr == 1) {
                if (is_load_vr(inst.name, inst.type_info.I.rs1, inst.type_info.I.imm, &vm) == VIR_R_CHAR) {
                    vm.registers[inst.type_info.I.rd] = extract_bits(r_char(), 7, 0);
                }
                else {
                    vm.registers[inst.type_info.I.rd] = extract_bits(r_int(), 7, 0);
                }
                load_vr = 0;
            }
            else {
                vm.registers[inst.type_info.I.rd] = get_rs_val(&vm, inst.type_info.I.rs1, inst.type_info.I.imm_signed);
            }             
        }
        if(strcmp("lhu", inst.name) == 0) {
            // unsigned 16 bit
            if(load_vr == 1) {
                if (is_load_vr(inst.name, inst.type_info.I.rs1, inst.type_info.I.imm, &vm) == VIR_R_CHAR) {
                    vm.registers[inst.type_info.I.rd] = extract_bits(r_char(), 15, 0);
                }
                else {
                    vm.registers[inst.type_info.I.rd] = extract_bits(r_int(), 15, 0);
                }
                load_vr = 0;
            }
            else {
                vm.registers[inst.type_info.I.rd] = get_mem_bytes(&vm, inst.type_info.I.rs1, inst.type_info.I.imm_signed, 2);
            }             
        }
        if(strcmp("sb", inst.name) == 0) {
            // 8 bit to mem
            vm.data_mem[vm.registers[inst.type_info.S.rs1] + inst.type_info.S.imm_signed - 0x0400] = (int32_t) vm.registers[inst.type_info.S.rs2]; 
        }
        if(strcmp("sh", inst.name) == 0) {
            // 16 bit to mem
            // vm.data_mem[vm.registers[inst.type_info.S.rs1] + inst.type_info.S.imm_signed - 0x0400] = vm.registers[inst.type_info.S.rs2]; 
            store_mem_bytes(&vm, inst.type_info.S.rs1, inst.type_info.S.imm_signed, vm.registers[inst.type_info.S.rs2], 2);
        }
        if(strcmp("sw", inst.name) == 0) {
            // 32 bit to mem
            store_mem_bytes(&vm, inst.type_info.S.rs1, inst.type_info.S.imm_signed, vm.registers[inst.type_info.S.rs2], 4);
        }
        
        // Program flow operations
        if (strcmp("slt", inst.name) == 0) {
            if((int32_t) vm.registers[inst.type_info.R.rs1] < (int32_t) vm.registers[inst.type_info.R.rs2]) {
                vm.registers[inst.type_info.R.rd] = 1;
            }
            else {
                vm.registers[inst.type_info.R.rd] = 0;
            }
        }
        if (strcmp("slti", inst.name) == 0) {
            // TODO implement signed
            if((int32_t)vm.registers[inst.type_info.I.rs1] < (int32_t) inst.type_info.I.imm_signed) {
                vm.registers[inst.type_info.I.rd] = 1;
            }
            else {
                vm.registers[inst.type_info.R.rd] = 0;
            }
        }
        if (strcmp("sltu", inst.name) == 0) {
            if(vm.registers[inst.type_info.R.rs1] < vm.registers[inst.type_info.R.rs2]) {
                vm.registers[inst.type_info.R.rd] = 1;
            }
            else {
                vm.registers[inst.type_info.R.rd] = 0;
            }
        }
        if (strcmp("sltiu", inst.name) == 0) {
            if(vm.registers[inst.type_info.I.rs1] < inst.type_info.I.imm) {
                vm.registers[inst.type_info.I.rd] = 1;
            }
            else {
                vm.registers[inst.type_info.I.rd] = 0;
            }
        }
        // these change PC values!!
        if (strcmp("beq", inst.name) == 0) {
            if((int32_t)vm.registers[inst.type_info.SB.rs1] == (int32_t) vm.registers[inst.type_info.SB.rs2]) {
                vm.PC += (inst.type_info.SB.imm_signed);
                continue;
            }
        }
        if(strcmp("bne", inst.name) == 0) {
            if((int32_t) vm.registers[inst.type_info.SB.rs1] != (int32_t) vm.registers[inst.type_info.SB.rs2]) {
                vm.PC += (inst.type_info.SB.imm_signed);
                continue;
            }
        }
        if(strcmp("blt", inst.name) == 0) {
            if((int32_t) vm.registers[inst.type_info.SB.rs1] < (int32_t) vm.registers[inst.type_info.SB.rs2]) {
                vm.PC += (inst.type_info.SB.imm_signed);
                continue;
            }
        }
        if(strcmp("bltu", inst.name) == 0) {
            // unsigned
            if(vm.registers[inst.type_info.SB.rs1] < vm.registers[inst.type_info.SB.rs2]) {
                vm.PC += (inst.type_info.SB.imm_signed);
                continue;
            }
        }
        if(strcmp("bge", inst.name) == 0) {
            if((int32_t) vm.registers[inst.type_info.SB.rs1] >= (int32_t) vm.registers[inst.type_info.SB.rs2]) {
                vm.PC += (inst.type_info.SB.imm_signed);
                continue;
            }
        }
        if(strcmp("bgeu", inst.name) == 0) {
            // unsigned
            if(vm.registers[inst.type_info.SB.rs1] >= vm.registers[inst.type_info.SB.rs2]) {
                vm.PC += (inst.type_info.SB.imm_signed);
                continue;
            }
        }
        if(strcmp("jal", inst.name) == 0) {
            vm.registers[inst.type_info.UJ.rd] = vm.PC + 4;
            vm.PC += (inst.type_info.UJ.imm_signed);

            continue;
        }
        if(strcmp("jalr", inst.name) == 0) {
            vm.registers[inst.type_info.I.rd] = vm.PC + 4;
            vm.PC = vm.registers[inst.type_info.I.rs1];
            continue;
        }

        
        //increment PC
        vm.PC += 4;

    }

    

    return 0;
}