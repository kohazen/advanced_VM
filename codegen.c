/*
 * codegen.c - AST to bytecode compiler (NEW for Lab 6)
 *
 * Compiles Lab 3 AST into Lab 4 VM bytecode, producing source-line
 * mappings consumed by the debugger (Lab 2 concepts).
 *
 * NOTE: We do NOT #include "instructions.h" here because its #define names
 * (OP_ADD, OP_SUB, etc.) collide with the OpType enum in ast.h.
 * Instead we use the bytecode hex values directly, matching instructions.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* Bytecode opcodes (hex values from Lab 4 instructions.h) */
#define EMIT_PUSH   0x01
#define EMIT_STORE  0x30
#define EMIT_LOAD   0x31
#define EMIT_ADD    0x10
#define EMIT_SUB    0x11
#define EMIT_MUL    0x12
#define EMIT_DIV    0x13
#define EMIT_CMP    0x14   /* a < b */
#define EMIT_CMP_EQ 0x15
#define EMIT_CMP_NE 0x16
#define EMIT_CMP_GT 0x17
#define EMIT_CMP_LE 0x18
#define EMIT_CMP_GE 0x19
#define EMIT_JMP    0x20
#define EMIT_JZ     0x21
#define EMIT_PRINT  0x50
#define EMIT_HALT   0xFF

static BytecodeProgram *prog;

static void emit_byte(uint8_t b) {
    if (prog->code_size >= MAX_CODE_SIZE) {
        fprintf(stderr, "codegen: code buffer overflow\n");
        exit(1);
    }
    prog->code[prog->code_size++] = b;
}

static void emit_int32(int32_t val) {
    emit_byte(val & 0xFF);
    emit_byte((val >> 8) & 0xFF);
    emit_byte((val >> 16) & 0xFF);
    emit_byte((val >> 24) & 0xFF);
}

static void patch_int32(int offset, int32_t val) {
    prog->code[offset]     = val & 0xFF;
    prog->code[offset + 1] = (val >> 8) & 0xFF;
    prog->code[offset + 2] = (val >> 16) & 0xFF;
    prog->code[offset + 3] = (val >> 24) & 0xFF;
}

static int current_offset(void) {
    return prog->code_size;
}

static void add_source_map(int line) {
    if (prog->source_map_count >= MAX_SOURCE_MAP) return;
    prog->source_map[prog->source_map_count].bytecode_offset = current_offset();
    prog->source_map[prog->source_map_count].source_line = line;
    prog->source_map_count++;
}

static int find_or_add_var(const char *name) {
    for (int i = 0; i < prog->var_count; i++) {
        if (strcmp(prog->var_names[i], name) == 0) return i;
    }
    if (prog->var_count >= MAX_CODEGEN_VARS) {
        fprintf(stderr, "codegen: too many variables\n");
        exit(1);
    }
    prog->var_names[prog->var_count] = strdup(name);
    return prog->var_count++;
}

static void codegen_node(ASTNode *node) {
    if (!node) return;

    if (node->line_number > 0) {
        add_source_map(node->line_number);
    }

    switch (node->type) {
        case NODE_INT:
            emit_byte(EMIT_PUSH);
            emit_int32(node->value);
            break;

        case NODE_VAR: {
            int slot = find_or_add_var(node->varName);
            emit_byte(EMIT_LOAD);
            emit_int32(slot);
            break;
        }

        case NODE_OP:
            codegen_node(node->left);
            codegen_node(node->right);
            switch (node->value) {
                case OP_ADD: emit_byte(EMIT_ADD); break;
                case OP_SUB: emit_byte(EMIT_SUB); break;
                case OP_MUL: emit_byte(EMIT_MUL); break;
                case OP_DIV: emit_byte(EMIT_DIV); break;
                case OP_LT:  emit_byte(EMIT_CMP); break;
                case OP_GT:  emit_byte(EMIT_CMP_GT); break;
                case OP_LE:  emit_byte(EMIT_CMP_LE); break;
                case OP_GE:  emit_byte(EMIT_CMP_GE); break;
                case OP_EQ:  emit_byte(EMIT_CMP_EQ); break;
                case OP_NEQ: emit_byte(EMIT_CMP_NE); break;
            }
            break;

        case NODE_DECL: {
            int slot = find_or_add_var(node->varName);
            if (node->left) {
                codegen_node(node->left);
            } else {
                emit_byte(EMIT_PUSH);
                emit_int32(0);
            }
            emit_byte(EMIT_STORE);
            emit_int32(slot);
            break;
        }

        case NODE_ASSIGN: {
            int slot = find_or_add_var(node->varName);
            codegen_node(node->left);
            emit_byte(EMIT_STORE);
            emit_int32(slot);
            break;
        }

        case NODE_PRINT:
            codegen_node(node->left);
            emit_byte(EMIT_PRINT);
            break;

        case NODE_IF: {
            codegen_node(node->left);  /* condition */
            emit_byte(EMIT_JZ);
            int jz_patch = current_offset();
            emit_int32(0);  /* placeholder */

            codegen_node(node->right);  /* then branch */

            if (node->extra) {
                emit_byte(EMIT_JMP);
                int jmp_patch = current_offset();
                emit_int32(0);
                patch_int32(jz_patch, current_offset());
                codegen_node(node->extra);  /* else branch */
                patch_int32(jmp_patch, current_offset());
            } else {
                patch_int32(jz_patch, current_offset());
            }
            break;
        }

        case NODE_WHILE: {
            int loop_start = current_offset();
            codegen_node(node->left);  /* condition */
            emit_byte(EMIT_JZ);
            int jz_patch = current_offset();
            emit_int32(0);

            codegen_node(node->right);  /* body */
            emit_byte(EMIT_JMP);
            emit_int32(loop_start);

            patch_int32(jz_patch, current_offset());
            break;
        }

        case NODE_SEQ:
            codegen_node(node->left);
            codegen_node(node->right);
            break;
    }
}

BytecodeProgram *codegen_compile(ASTNode *root) {
    prog = calloc(1, sizeof(BytecodeProgram));
    prog->code = malloc(MAX_CODE_SIZE);
    prog->code_size = 0;
    prog->var_count = 0;
    prog->source_map_count = 0;

    codegen_node(root);
    emit_byte(EMIT_HALT);

    BytecodeProgram *result = prog;
    prog = NULL;
    return result;
}

void codegen_free(BytecodeProgram *p) {
    if (!p) return;
    for (int i = 0; i < p->var_count; i++) free(p->var_names[i]);
    free(p->code);
    free(p);
}

int codegen_line_for_pc(BytecodeProgram *p, int pc) {
    int best_line = 0;
    for (int i = 0; i < p->source_map_count; i++) {
        if (p->source_map[i].bytecode_offset <= pc) {
            best_line = p->source_map[i].source_line;
        }
    }
    return best_line;
}

int codegen_pc_for_line(BytecodeProgram *p, int line) {
    for (int i = 0; i < p->source_map_count; i++) {
        if (p->source_map[i].source_line == line) {
            return p->source_map[i].bytecode_offset;
        }
    }
    return -1;
}

const char *codegen_var_name(BytecodeProgram *p, int slot) {
    if (slot < 0 || slot >= p->var_count) return NULL;
    return p->var_names[slot];
}

int codegen_var_slot(BytecodeProgram *p, const char *name) {
    for (int i = 0; i < p->var_count; i++) {
        if (strcmp(p->var_names[i], name) == 0) return i;
    }
    return -1;
}
