#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugger_vm.h"
#include "instructions.h"

Debugger *debugger_create(VM *vm, BytecodeProgram *prog) {
    Debugger *dbg = calloc(1, sizeof(Debugger));
    dbg->vm = vm;
    dbg->prog = prog;
    dbg->bp_count = 0;
    dbg->last_line = 0;
    return dbg;
}

void debugger_destroy(Debugger *dbg) {
    free(dbg);
}

void debugger_add_breakpoint(Debugger *dbg, int line) {
    if (dbg->bp_count >= MAX_BREAKPOINTS) {
        printf("Max breakpoints reached\n");
        return;
    }
    for (int i = 0; i < dbg->bp_count; i++) {
        if (dbg->breakpoints[i] == line) {
            printf("Breakpoint already set at line %d\n", line);
            return;
        }
    }
    int pc = codegen_pc_for_line(dbg->prog, line);
    if (pc < 0) {
        printf("No code at line %d\n", line);
        return;
    }
    dbg->breakpoints[dbg->bp_count++] = line;
    printf("Breakpoint set at line %d (pc=%d)\n", line, pc);
}

void debugger_remove_breakpoint(Debugger *dbg, int line) {
    for (int i = 0; i < dbg->bp_count; i++) {
        if (dbg->breakpoints[i] == line) {
            dbg->breakpoints[i] = dbg->breakpoints[--dbg->bp_count];
            printf("Breakpoint removed at line %d\n", line);
            return;
        }
    }
    printf("No breakpoint at line %d\n", line);
}

void debugger_list_breakpoints(Debugger *dbg) {
    if (dbg->bp_count == 0) {
        printf("No breakpoints set\n");
        return;
    }
    printf("Breakpoints:\n");
    for (int i = 0; i < dbg->bp_count; i++) {
        printf("  line %d\n", dbg->breakpoints[i]);
    }
}

static int is_breakpoint(Debugger *dbg, int line) {
    for (int i = 0; i < dbg->bp_count; i++) {
        if (dbg->breakpoints[i] == line) return 1;
    }
    return 0;
}

void debugger_step_instruction(Debugger *dbg) {
    if (!dbg->vm->running && dbg->vm->pc < dbg->vm->code_size) {
        dbg->vm->running = true;
        dbg->vm->error = VM_OK;
    }
    if (dbg->vm->running) {
        vm_step(dbg->vm);
        int line = codegen_line_for_pc(dbg->prog, dbg->vm->pc);
        if (line > 0) dbg->last_line = line;
        printf("  PC=%d (line %d)\n", dbg->vm->pc, dbg->last_line);
    } else {
        printf("Program has halted\n");
    }
}

void debugger_step_line(Debugger *dbg) {
    if (!dbg->vm->running && dbg->vm->pc < dbg->vm->code_size) {
        dbg->vm->running = true;
        dbg->vm->error = VM_OK;
    }
    int start_line = codegen_line_for_pc(dbg->prog, dbg->vm->pc);
    while (dbg->vm->running) {
        vm_step(dbg->vm);
        int cur_line = codegen_line_for_pc(dbg->prog, dbg->vm->pc);
        if (cur_line != start_line && cur_line > 0) {
            dbg->last_line = cur_line;
            break;
        }
    }
    if (!dbg->vm->running) {
        printf("Program halted at PC=%d\n", dbg->vm->pc);
    } else {
        printf("  Stopped at line %d (PC=%d)\n", dbg->last_line, dbg->vm->pc);
    }
}

void debugger_continue(Debugger *dbg) {
    if (!dbg->vm->running && dbg->vm->pc < dbg->vm->code_size) {
        dbg->vm->running = true;
        dbg->vm->error = VM_OK;
    }
    /* Step past current position first to avoid re-triggering same breakpoint */
    if (dbg->vm->running) {
        vm_step(dbg->vm);
    }
    while (dbg->vm->running) {
        int line = codegen_line_for_pc(dbg->prog, dbg->vm->pc);
        if (line > 0 && is_breakpoint(dbg, line) && line != dbg->last_line) {
            dbg->last_line = line;
            printf("Hit breakpoint at line %d (PC=%d)\n", line, dbg->vm->pc);
            return;
        }
        if (line > 0) dbg->last_line = line;
        vm_step(dbg->vm);
    }
    printf("Program finished\n");
}

void debugger_print_regs(Debugger *dbg) {
    printf("PC:  %d\n", dbg->vm->pc);
    printf("SP:  %d\n", dbg->vm->sp);
    printf("RSP: %d\n", dbg->vm->rsp);
    int line = codegen_line_for_pc(dbg->prog, dbg->vm->pc);
    printf("Line: %d\n", line);
    printf("Running: %s\n", dbg->vm->running ? "yes" : "no");
}

void debugger_print_stack(Debugger *dbg) {
    if (dbg->vm->sp == 0) {
        printf("Stack is empty\n");
        return;
    }
    printf("Stack (top first):\n");
    for (int i = dbg->vm->sp - 1; i >= 0; i--) {
        printf("  [%d] = %d\n", i, dbg->vm->stack[i]);
    }
}

void debugger_print_vars(Debugger *dbg) {
    if (dbg->prog->var_count == 0) {
        printf("No variables\n");
        return;
    }
    printf("Variables:\n");
    for (int i = 0; i < dbg->prog->var_count; i++) {
        printf("  %s = %d (slot %d)\n",
               dbg->prog->var_names[i], dbg->vm->memory[i], i);
    }
}

void debugger_print_memstat(Debugger *dbg) {
    printf("GC Objects: %d\n", dbg->vm->num_objects);
    printf("GC Threshold: %d\n", dbg->vm->max_objects);
    printf("Auto GC: %s\n", dbg->vm->auto_gc ? "enabled" : "disabled");
}

void debugger_interactive(Debugger *dbg) {
    char line[256];

    /* Start VM */
    dbg->vm->running = true;
    dbg->vm->error = VM_OK;
    dbg->last_line = codegen_line_for_pc(dbg->prog, 0);

    printf("Debugger ready. Type 'help' for commands.\n");
    printf("Program loaded: %d bytes, %d variables\n",
           dbg->prog->code_size, dbg->prog->var_count);

    while (1) {
        printf("dbg> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue;

        if (strcmp(line, "help") == 0) {
            printf("Commands:\n");
            printf("  break <line>   - set breakpoint at source line\n");
            printf("  delete <line>  - remove breakpoint\n");
            printf("  list           - list breakpoints\n");
            printf("  step           - step one instruction\n");
            printf("  next           - step one source line\n");
            printf("  continue       - run until breakpoint or end\n");
            printf("  regs           - show PC, SP, RSP\n");
            printf("  stack          - show stack contents\n");
            printf("  vars           - show variable values\n");
            printf("  memstat        - show GC statistics\n");
            printf("  quit           - exit debugger\n");
        } else if (strncmp(line, "break ", 6) == 0) {
            int ln = atoi(line + 6);
            debugger_add_breakpoint(dbg, ln);
        } else if (strncmp(line, "delete ", 7) == 0) {
            int ln = atoi(line + 7);
            debugger_remove_breakpoint(dbg, ln);
        } else if (strcmp(line, "list") == 0) {
            debugger_list_breakpoints(dbg);
        } else if (strcmp(line, "step") == 0 || strcmp(line, "s") == 0) {
            debugger_step_instruction(dbg);
        } else if (strcmp(line, "next") == 0 || strcmp(line, "n") == 0) {
            debugger_step_line(dbg);
        } else if (strcmp(line, "continue") == 0 || strcmp(line, "c") == 0) {
            debugger_continue(dbg);
        } else if (strcmp(line, "regs") == 0) {
            debugger_print_regs(dbg);
        } else if (strcmp(line, "stack") == 0) {
            debugger_print_stack(dbg);
        } else if (strcmp(line, "vars") == 0) {
            debugger_print_vars(dbg);
        } else if (strcmp(line, "memstat") == 0) {
            debugger_print_memstat(dbg);
        } else if (strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
            printf("Exiting debugger\n");
            break;
        } else {
            printf("Unknown command: %s (type 'help')\n", line);
        }
    }
}
