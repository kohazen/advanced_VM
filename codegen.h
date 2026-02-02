/*
 * codegen.h - AST to bytecode compiler (NEW for Lab 6)
 *
 * This module fulfills the Lab 3 "Intermediate Representation" role:
 * compiles the AST (from parser) into VM bytecode (for Lab 4 VM),
 * with source-line mapping (for Lab 2 debugger).
 */
#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include "ast.h"

#define MAX_CODE_SIZE 4096
#define MAX_CODEGEN_VARS 128
#define MAX_SOURCE_MAP 1024

typedef struct {
    int bytecode_offset;
    int source_line;
} SourceMapEntry;

typedef struct {
    uint8_t *code;
    int code_size;

    char *var_names[MAX_CODEGEN_VARS];
    int var_count;

    SourceMapEntry source_map[MAX_SOURCE_MAP];
    int source_map_count;
} BytecodeProgram;

BytecodeProgram *codegen_compile(ASTNode *root);
void codegen_free(BytecodeProgram *prog);

int codegen_line_for_pc(BytecodeProgram *prog, int pc);
int codegen_pc_for_line(BytecodeProgram *prog, int line);

const char *codegen_var_name(BytecodeProgram *prog, int slot);
int codegen_var_slot(BytecodeProgram *prog, const char *name);

#endif
