#ifndef VIR_ROUTINE_H_
#define VIR_ROUTINE_H_
#include "structs_enums.h"

uint32_t is_store_vr(char *inst_name, uint8_t rs1, int imm, struct VM *vm);
uint32_t is_load_vr(char *inst_name, uint8_t rs1, int imm, struct VM *vm);
int get_num_bits(char *inst_name);
void w_char(uint32_t value, char *inst_name);
void w_int(uint32_t value, char *inst_name);
void w_uint(uint32_t value, char *inst_name);
void halt();
void dump_PC(uint16_t PC);
void dump_reg(struct VM *vm);
void dump_mem(uint32_t value);
uint32_t r_char();
int32_t r_int();

#endif
