#ifndef PROGRAM_MANAGER_H
#define PROGRAM_MANAGER_H

#include "codegen.h"
#include "vm.h"

#define MAX_PROGRAMS 64

typedef enum {
    PROG_SUBMITTED,
    PROG_RUNNING,
    PROG_PAUSED,
    PROG_FINISHED,
    PROG_ERROR
} ProgramState;

typedef struct {
    int pid;
    char *filename;
    ProgramState state;
    BytecodeProgram *bytecode;
    VM *vm;
} ProgramEntry;

typedef struct {
    ProgramEntry programs[MAX_PROGRAMS];
    int count;
    int next_pid;
} ProgramManager;

ProgramManager *pm_create(void);
void pm_destroy(ProgramManager *pm);

int pm_submit(ProgramManager *pm, const char *filename);
int pm_run(ProgramManager *pm, int pid);
int pm_debug(ProgramManager *pm, int pid);
int pm_kill(ProgramManager *pm, int pid);
int pm_memstat(ProgramManager *pm, int pid);
int pm_gc(ProgramManager *pm, int pid);
int pm_leaks(ProgramManager *pm, int pid);
void pm_list(ProgramManager *pm);

#endif
