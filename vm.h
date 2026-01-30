#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>
#include "gc.h"  /* For Object and Value types */

#define STACK_SIZE        1024
#define MEMORY_SIZE       256
#define RETURN_STACK_SIZE 256
#define VM_STACK_MAX      256

typedef enum {
    VM_OK = 0,
    VM_ERROR_STACK_OVERFLOW,
    VM_ERROR_STACK_UNDERFLOW,
    VM_ERROR_INVALID_OPCODE,
    VM_ERROR_DIVISION_BY_ZERO,
    VM_ERROR_MEMORY_BOUNDS,
    VM_ERROR_CODE_BOUNDS,
    VM_ERROR_RETURN_STACK_OVERFLOW,
    VM_ERROR_RETURN_STACK_UNDERFLOW,
    VM_ERROR_FILE_IO
} VMError;

typedef struct VM {
    /* Original VM fields */
    int32_t *stack;
    int sp;
    int32_t *memory;
    uint8_t *code;
    int code_size;
    int pc;
    int32_t *return_stack;
    int rsp;
    bool running;
    VMError error;

    /* GC-related fields (Lab 5) */
    Object *first_object;
    int num_objects;
    int max_objects;
    Value *value_stack;
    int stack_count;
    bool auto_gc;  /* Enable/disable automatic GC triggering */
} VM;

VM* vm_create(void);
void vm_destroy(VM *vm);
VMError vm_load_program(VM *vm, uint8_t *bytecode, int size);
VMError vm_run(VM *vm);
void vm_dump_state(VM *vm);
const char* vm_error_string(VMError error);

/* LAB6 CHANGE: single-step execution for debugger support */
VMError vm_step(VM *vm);

#endif
