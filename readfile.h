#ifndef READ_FILE_H_
#define READ_FILE_H_

#include "structs_enums.h"

void read_file_into_mem(struct VM *vm, char *filename);

void get_inst_lines(struct VM *vm, char *filename);

#endif