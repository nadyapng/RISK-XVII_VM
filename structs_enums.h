#ifndef STRUCTS_ENUMS_H_
#define STRUCTS_ENUMS_H_
#include <stdint.h>

#define INST_MEM_SIZE 1024
#define DATA_MEM_SIZE 1024
#define HEAP_BANK_NUM 128
#define HEAP_BANK_SIZE 64

struct HEAP_BANK{
    uint8_t heap_data[64];
    int bytes_allocated;
};

struct VM {
    uint8_t inst_mem[INST_MEM_SIZE];     // 1024/4 to as 4 bytes (32 bits) read in at once
    uint32_t inst_lines[INST_MEM_SIZE/4];   // saves full lines of instruction
    uint8_t data_mem[DATA_MEM_SIZE];
    struct HEAP_BANK heap[HEAP_BANK_NUM];
    uint32_t registers[32];
    uint16_t PC;    
    uint16_t PC_lines;  // PC for inst_lines

};

enum TYPE {
    TYPE_R     = 0b0110011, 
    TYPE_I     = 0b0010011,
    TYPE_I_LOAD = 0b0000011,
    TYPE_I_JMP = 0b1100111,
    TYPE_U     = 0b0110111,
    TYPE_S     = 0b0100011,
    TYPE_SB    = 0b1100011,
    TYPE_UJ    = 0b1101111,
    TYPE_INVALID
};


enum R_FUNC7 {
    ADD_FUNC7 = 0b0000000,
    SUB_FUNC7 = 0b0100000,
    SRL_FUNC7 = 0b0000000,
    SRA_FUNC7 = 0b0100000
};

enum FUNC3 {
    ADD_FUNC3   = 0b000,
    SUB_FUNC3   = 0b000,
    XOR_FUNC3   = 0b100,
    OR_FUNC3    = 0b110,
    AND_FUNC3   = 0b111,
    SLL_FUNC3   = 0b001,
    SRL_FUNC3   = 0b101,
    SRA_FUNC3   = 0b101,
    SLT_FUNC3   = 0b010,
    SLTU_FUNC3  = 0b011,
    ADDI_FUNC3  = 0b000,
    XORI_FUNC3  = 0b100,
    ORI_FUNC3   = 0b110,
    ANDI_FUNC3  = 0b111,
    SLTI_FUNC3  = 0b010,
    SLTIU_FUNC3 = 0b011,
    LB_FUNC3    = 0b000,
    LH_FUNC3    = 0b001,
    LW_FUNC3    = 0b010,
    LBU_FUNC3   = 0b100,
    LHU_FUNC3   = 0b101,
    JALR_FUNC3  = 0b000,
    SB_FUNC3    = 0b000,
    SH_FUNC3    = 0b001,
    SW_FUNC3    = 0b010,
    BEQ_FUNC3   = 0b000,
    BNE_FUNC3   = 0b001,
    BLT_FUNC3   = 0b100,
    BLTU_FUNC3  = 0b110,
    BGE_FUNC3   = 0b101,
    BGEU_FUNC3   = 0b111
};

enum VIR_ROUTINE {

    VIR_W_CHAR   = 0x0800,
    VIR_W_INT    = 0x0804,
    VIR_W_UINT   = 0x0808,
    VIR_HALT     = 0x080C,
    VIR_R_CHAR   = 0x0812,
    VIR_R_INT    = 0x0816,
    VIR_DUMP_PC  = 0x0820,
    VIR_DUMP_REG = 0x0824,
    VIR_DUMP_MEM = 0x0828

};

enum HEAP {
    
    HEAP_MALLOC = 0x0830,
    HEAP_FREE = 0x0834,

};

struct INST {
    uint32_t line;
    uint8_t opcode;
    enum TYPE type;
    char *name;
    union {
        struct INST_R {
            uint8_t rd;
            uint8_t func3;
            uint8_t rs1;
            uint8_t rs2;
            uint8_t func7;
        }R;
        struct INST_I {
            uint8_t rd;
            uint8_t func3;
            uint8_t rs1;
            uint32_t imm; 
            int32_t imm_signed;
        }I;
        struct INST_S {
            uint32_t imm1;
            uint8_t func3;
            uint8_t rs1;
            uint8_t rs2;
            uint32_t imm2;
            uint32_t imm;
            int32_t imm_signed;
        }S;
        struct INST_SB {
            uint32_t imm1;
            uint8_t func3;
            uint8_t rs1;
            uint8_t rs2;
            uint32_t imm2;
            uint32_t imm;
            int32_t imm_signed;
        }SB;
        struct INST_U {
            uint8_t rd;
            uint32_t imm;
            int32_t imm_signed;
        }U;
        struct INST_UJ {
            uint8_t rd;
            uint32_t imm;
            int32_t imm_signed;
        }UJ;
    }type_info;
};
#endif