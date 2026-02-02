#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "program_manager.h"
#include "debugger_vm.h"
#include "ast.h"

/* Parser interface */
extern ASTNode *root;
extern int yylineno;
extern FILE *yyin;
int yyparse(void);
int yylex_destroy(void);

ProgramManager *pm_create(void) {
    ProgramManager *pm = calloc(1, sizeof(ProgramManager));
    pm->next_pid = 1;
    return pm;
}

void pm_destroy(ProgramManager *pm) {
    for (int i = 0; i < pm->count; i++) {
        if (pm->programs[i].filename) free(pm->programs[i].filename);
        if (pm->programs[i].bytecode) codegen_free(pm->programs[i].bytecode);
        if (pm->programs[i].vm) vm_destroy(pm->programs[i].vm);
    }
    free(pm);
}

static ProgramEntry *find_program(ProgramManager *pm, int pid) {
    for (int i = 0; i < pm->count; i++) {
        if (pm->programs[i].pid == pid) return &pm->programs[i];
    }
    return NULL;
}

static const char *state_str(ProgramState s) {
    switch (s) {
        case PROG_SUBMITTED: return "SUBMITTED";
        case PROG_RUNNING:   return "RUNNING";
        case PROG_PAUSED:    return "PAUSED";
        case PROG_FINISHED:  return "FINISHED";
        case PROG_ERROR:     return "ERROR";
    }
    return "UNKNOWN";
}

int pm_submit(ProgramManager *pm, const char *filename) {
    if (pm->count >= MAX_PROGRAMS) {
        fprintf(stderr, "Error: max programs reached\n");
        return -1;
    }

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", filename);
        return -1;
    }

    /* Parse */
    root = NULL;
    yylineno = 1;
    yyin = f;
    int result = yyparse();
    fclose(f);
    yylex_destroy();

    if (result != 0 || !root) {
        fprintf(stderr, "Error: parse failed for '%s'\n", filename);
        return -1;
    }

    /* Compile */
    BytecodeProgram *bc = codegen_compile(root);
    ast_free(root);
    root = NULL;

    if (!bc) {
        fprintf(stderr, "Error: codegen failed for '%s'\n", filename);
        return -1;
    }

    int pid = pm->next_pid++;
    ProgramEntry *entry = &pm->programs[pm->count++];
    entry->pid = pid;
    entry->filename = strdup(filename);
    entry->state = PROG_SUBMITTED;
    entry->bytecode = bc;
    entry->vm = NULL;

    printf("Program '%s' submitted as PID %d (%d bytes bytecode, %d vars)\n",
           filename, pid, bc->code_size, bc->var_count);
    return pid;
}

int pm_run(ProgramManager *pm, int pid) {
    ProgramEntry *e = find_program(pm, pid);
    if (!e) { fprintf(stderr, "Error: PID %d not found\n", pid); return -1; }
    if (e->state != PROG_SUBMITTED) {
        fprintf(stderr, "Error: PID %d is %s (must be SUBMITTED)\n", pid, state_str(e->state));
        return -1;
    }

    VM *vm = vm_create();
    if (!vm) { fprintf(stderr, "Error: vm_create failed\n"); return -1; }

    /* Copy bytecode so VM doesn't own it */
    uint8_t *code_copy = malloc(e->bytecode->code_size);
    memcpy(code_copy, e->bytecode->code, e->bytecode->code_size);
    vm_load_program(vm, code_copy, e->bytecode->code_size);

    e->vm = vm;
    e->state = PROG_RUNNING;

    printf("Running PID %d...\n", pid);
    VMError err = vm_run(vm);

    if (err == VM_OK) {
        e->state = PROG_FINISHED;
        printf("PID %d finished successfully\n", pid);
    } else {
        e->state = PROG_ERROR;
        fprintf(stderr, "PID %d error: %s\n", pid, vm_error_string(err));
    }
    return 0;
}

int pm_debug(ProgramManager *pm, int pid) {
    ProgramEntry *e = find_program(pm, pid);
    if (!e) { fprintf(stderr, "Error: PID %d not found\n", pid); return -1; }
    if (e->state != PROG_SUBMITTED && e->state != PROG_FINISHED && e->state != PROG_ERROR) {
        fprintf(stderr, "Error: PID %d is %s\n", pid, state_str(e->state));
        return -1;
    }

    VM *vm = vm_create();
    if (!vm) { fprintf(stderr, "Error: vm_create failed\n"); return -1; }

    uint8_t *code_copy = malloc(e->bytecode->code_size);
    memcpy(code_copy, e->bytecode->code, e->bytecode->code_size);
    vm_load_program(vm, code_copy, e->bytecode->code_size);

    if (e->vm) vm_destroy(e->vm);
    e->vm = vm;
    e->state = PROG_PAUSED;

    Debugger *dbg = debugger_create(vm, e->bytecode);
    debugger_interactive(dbg);
    debugger_destroy(dbg);

    if (!vm->running) {
        e->state = PROG_FINISHED;
    }
    return 0;
}

int pm_kill(ProgramManager *pm, int pid) {
    ProgramEntry *e = find_program(pm, pid);
    if (!e) { fprintf(stderr, "Error: PID %d not found\n", pid); return -1; }

    if (e->vm) {
        e->vm->running = false;
        vm_destroy(e->vm);
        e->vm = NULL;
    }
    e->state = PROG_FINISHED;
    printf("PID %d killed\n", pid);
    return 0;
}

int pm_memstat(ProgramManager *pm, int pid) {
    ProgramEntry *e = find_program(pm, pid);
    if (!e) { fprintf(stderr, "Error: PID %d not found\n", pid); return -1; }
    if (!e->vm) { fprintf(stderr, "Error: PID %d has no VM instance\n", pid); return -1; }

    printf("=== Memory Stats for PID %d ===\n", pid);
    printf("GC Objects:    %d\n", e->vm->num_objects);
    printf("GC Threshold:  %d\n", e->vm->max_objects);
    printf("Auto GC:       %s\n", e->vm->auto_gc ? "enabled" : "disabled");
    printf("Stack Depth:   %d\n", e->vm->sp);
    printf("Memory Slots:  %d used\n", e->bytecode->var_count);
    return 0;
}

int pm_gc(ProgramManager *pm, int pid) {
    ProgramEntry *e = find_program(pm, pid);
    if (!e) { fprintf(stderr, "Error: PID %d not found\n", pid); return -1; }
    if (!e->vm) { fprintf(stderr, "Error: PID %d has no VM instance\n", pid); return -1; }

    printf("Forcing GC on PID %d...\n", pid);
    gc_collect(e->vm);
    return 0;
}

int pm_leaks(ProgramManager *pm, int pid) {
    ProgramEntry *e = find_program(pm, pid);
    if (!e) { fprintf(stderr, "Error: PID %d not found\n", pid); return -1; }
    if (!e->vm) { fprintf(stderr, "Error: PID %d has no VM instance\n", pid); return -1; }

    if (e->vm->num_objects == 0) {
        printf("PID %d: No leaks detected (0 objects on heap)\n", pid);
    } else {
        printf("PID %d: %d objects still on heap\n", pid, e->vm->num_objects);
        Object *obj = e->vm->first_object;
        int count = 0;
        while (obj && count < 10) {
            const char *type_str = "unknown";
            switch (obj->type) {
                case OBJ_PAIR: type_str = "pair"; break;
                case OBJ_FUNCTION: type_str = "function"; break;
                case OBJ_CLOSURE: type_str = "closure"; break;
            }
            printf("  [%d] type=%s marked=%s\n", count, type_str, obj->marked ? "yes" : "no");
            obj = obj->next;
            count++;
        }
        if (e->vm->num_objects > 10) {
            printf("  ... and %d more\n", e->vm->num_objects - 10);
        }
    }
    return 0;
}

void pm_list(ProgramManager *pm) {
    if (pm->count == 0) {
        printf("No programs submitted\n");
        return;
    }
    printf("PID  State       File\n");
    printf("---  ----------  ----\n");
    for (int i = 0; i < pm->count; i++) {
        printf("%-4d %-10s  %s\n",
               pm->programs[i].pid,
               state_str(pm->programs[i].state),
               pm->programs[i].filename);
    }
}
