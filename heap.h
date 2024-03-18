#ifndef HEAP_H_
#define HEAP_H_
#include <stdint.h>
#include "structs_enums.h"
uint32_t is_heap(char *inst_name, uint8_t rs1, int imm, struct VM *vm);
uint32_t malloc_heap(struct VM *vm, uint32_t num_bytes);
int exe_heap(struct INST *inst, struct VM *vm);
#endif
