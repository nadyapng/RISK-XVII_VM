#ifndef PARSE_H_
#define PARSE_H_
#include <stdint.h>
#include "structs_enums.h"
uint32_t extract_bits(uint32_t instruction, int start_index, int end_index);

uint8_t extract_opcode(uint32_t instruction);

uint8_t extract_opcode(uint32_t instruction);

uint8_t extract_func3(uint32_t instruction);

uint8_t extract_rs1(uint32_t instruction);

uint8_t extract_rs2(uint32_t instruction);

uint8_t extract_func7(uint32_t instruction);

int is_negative(uint32_t imm, int length);

uint32_t get_2s_comp(uint32_t imm, int length);

int convert_2s_comp(uint32_t imm, int length);

enum TYPE inst_type(uint8_t opcode);

#endif
