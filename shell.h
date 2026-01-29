/*
 * shell.h - Shell interface (NEW for Lab 6)
 *
 * Wraps Lab 1 myshell.c main loop into a callable function,
 * passing ProgramManager for new builtin commands.
 */
#ifndef SHELL_H
#define SHELL_H

#include "program_manager.h"

void shell_run(ProgramManager *pm);

#endif
