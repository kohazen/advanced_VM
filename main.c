#include <stdio.h>
#include "shell.h"
#include "program_manager.h"

int main(void) {
    ProgramManager *pm = pm_create();
    shell_run(pm);
    pm_destroy(pm);
    return 0;
}
