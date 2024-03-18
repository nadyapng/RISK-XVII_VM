#ifndef STORE_LOAD_HELPER_H_
#define STORE_LOAD_HELPER_H_

#include "structs_enums.h"

void store_mem_bytes(struct VM *vm, uint8_t rs1, int imm, uint32_t val, int num_bytes);

int32_t sext(uint32_t val, int num_bits);

uint32_t get_rs_val(struct VM *vm, uint8_t rs1, int imm);

uint32_t get_mem_bytes(struct VM *vm, uint8_t rs1, int imm, int num_bytes);

#endif