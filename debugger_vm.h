#ifndef DEBUGGER_VM_H
#define DEBUGGER_VM_H

#include "vm.h"
#include "codegen.h"

#define MAX_BREAKPOINTS 32

typedef struct {
    VM *vm;
    BytecodeProgram *prog;

    int breakpoints[MAX_BREAKPOINTS];
    int bp_count;

    int last_line;
} Debugger;

Debugger *debugger_create(VM *vm, BytecodeProgram *prog);
void debugger_destroy(Debugger *dbg);

void debugger_add_breakpoint(Debugger *dbg, int line);
void debugger_remove_breakpoint(Debugger *dbg, int line);
void debugger_list_breakpoints(Debugger *dbg);

void debugger_step_instruction(Debugger *dbg);
void debugger_step_line(Debugger *dbg);
void debugger_continue(Debugger *dbg);

void debugger_print_regs(Debugger *dbg);
void debugger_print_stack(Debugger *dbg);
void debugger_print_vars(Debugger *dbg);
void debugger_print_memstat(Debugger *dbg);

void debugger_interactive(Debugger *dbg);

#endif
