# Lab 6: Integrated Programming Assignment

**End-to-End Execution, Debugging, and Memory Management Framework**

This project integrates Labs 1--5 into a single, cohesive system that manages the complete
lifecycle of a user program: submission, parsing, compilation, execution, debugging, and
memory management. Every lab component is essential -- removing any one breaks the system.

```
Program  -->  Parse & Instrument  -->  Compile to IR  -->  Execute on VM  -->  Debug & Trace  -->  Memory Accounting
(Shell)       (Parser/AST)             (Codegen)           (VM)               (Debugger)          (GC)
 Lab 1         Lab 3                    Lab 3 IR            Lab 4              Lab 2               Lab 5
```

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building](#building)
3. [Running the System](#running-the-system)
4. [Shell Commands Reference](#shell-commands-reference)
5. [Debugger Commands Reference](#debugger-commands-reference)
6. [Source Language (.lang)](#source-language-lang)
7. [Architecture Overview](#architecture-overview)
8. [File-by-File Breakdown](#file-by-file-breakdown)
9. [What Was Taken from Each Lab](#what-was-taken-from-each-lab)
10. [What Was Changed for Integration](#what-was-changed-for-integration)
11. [Data Flow Between Components](#data-flow-between-components)
12. [Test Programs](#test-programs)
13. [Example Sessions](#example-sessions)

---

## Prerequisites

- **Linux** (tested on Ubuntu 24.04)
- GCC (C11 or later)
- GNU Flex (lexer generator)
- GNU Bison >= 3.0 (parser generator)

Install on Debian/Ubuntu:

```bash
sudo apt install build-essential flex bison
```

---

## Building

```bash
make clean && make
```

This runs:
1. `bison -d parser.y` -- generates `parser.tab.c` and `parser.tab.h`
2. `flex lexer.l` -- generates `lex.yy.c`
3. `gcc` -- compiles all source files and links them into the `lab6shell` binary

The build produces **zero warnings** with `-Wall -Wextra`.

---

## Running the System

```bash
./lab6shell
```

This launches an interactive shell with the `myshell>` prompt. The shell supports both
the integrated lab commands (submit, run, debug, etc.) and standard Unix commands via
fork/exec, including pipes, I/O redirection, background processes, and semicolon-separated
command chaining.

---

## Shell Commands Reference

### Integrated Lab Commands

| Command          | Description                                           |
|------------------|-------------------------------------------------------|
| `submit <file>`  | Parse and compile a `.lang` file; assigns a PID       |
| `run <pid>`      | Execute a submitted program on the VM                 |
| `debug <pid>`    | Launch interactive debugger for a program             |
| `kill <pid>`     | Terminate a program and destroy its VM instance       |
| `memstat <pid>`  | Print GC object count, threshold, stack depth, vars   |
| `gc <pid>`       | Force a garbage collection cycle on a program's VM    |
| `leaks <pid>`    | Report heap objects still alive (up to 10 shown)      |
| `ps`             | List all submitted programs with PID, state, filename |

### Program States

| State       | Meaning                                    |
|-------------|--------------------------------------------|
| `SUBMITTED` | Parsed and compiled, ready to run or debug |
| `RUNNING`   | Currently executing on the VM              |
| `PAUSED`    | In debugger, execution suspended           |
| `FINISHED`  | Execution completed successfully           |
| `ERROR`     | Execution terminated with a VM error       |

### Original Shell Commands (from Lab 1)

| Command      | Description                                    |
|--------------|------------------------------------------------|
| `cd <dir>`   | Change working directory                       |
| `exit`       | Exit the shell                                 |
| `<cmd> &`    | Run external command in background             |
| `<cmd> \| <cmd>` | Pipe output of one command to another     |
| `<cmd> < in` | Redirect stdin from file                       |
| `<cmd> > out`| Redirect stdout to file                        |
| `cmd1 ; cmd2`| Run multiple commands sequentially              |

Any command not recognized as a builtin is executed as an external program via `fork()`/`execvp()`.

---

## Debugger Commands Reference

When you run `debug <pid>`, the system enters an interactive debugger (`dbg>` prompt)
with the following commands:

| Command          | Shortcut | Description                                    |
|------------------|----------|------------------------------------------------|
| `break <line>`   |          | Set breakpoint at a source line number         |
| `delete <line>`  |          | Remove a breakpoint                            |
| `list`           |          | List all active breakpoints                    |
| `step`           | `s`      | Step one VM instruction (bytecode-level)       |
| `next`           | `n`      | Step one source line (skips multiple bytecodes)|
| `continue`       | `c`      | Run until next breakpoint or program end       |
| `regs`           |          | Show PC, SP, RSP, current source line          |
| `stack`          |          | Show VM stack contents (top first)             |
| `vars`           |          | Show all variable names and current values     |
| `memstat`        |          | Show GC object count and threshold             |
| `quit`           | `q`      | Exit debugger, return to shell                 |
| `help`           |          | Show command reference                         |

The debugger uses **source-level line mapping**: breakpoints are set on source lines, and
`next` steps by source line. The `step` command operates at the bytecode instruction level.
Variable inspection shows symbolic names mapped back from memory slots.

---

## Source Language (.lang)

The `.lang` language is a simple imperative language with the following features:

### Data Types

- **Integer**: 32-bit signed integers (e.g., `42`, `0`, `-5`)

### Variables

```
var x = 10;       // declaration with initialization
var y;             // declaration with default value 0
x = x + 1;        // assignment
```

### Arithmetic Operators

| Operator | Meaning        |
|----------|----------------|
| `+`      | Addition       |
| `-`      | Subtraction    |
| `*`      | Multiplication |
| `/`      | Integer division |

### Comparison Operators

| Operator | Meaning              |
|----------|----------------------|
| `==`     | Equal                |
| `!=`     | Not equal            |
| `<`      | Less than            |
| `>`      | Greater than         |
| `<=`     | Less than or equal   |
| `>=`     | Greater than or equal|

Comparisons return `1` (true) or `0` (false).

### Control Flow

```
if (x < 10) {
    print(x);
} else {
    print(0);
}

while (i < 10) {
    i = i + 1;
}
```

### Output

```
print(expression);    // prints the integer value followed by a newline
```

### Syntax Rules

- All statements end with a semicolon (`;`)
- Blocks use curly braces (`{ ... }`)
- Parentheses required around `if`/`while` conditions
- Standard operator precedence: `*`/`/` before `+`/`-` before comparisons

---

## Architecture Overview

The system is organized as a pipeline where each component feeds into the next:

```
                    +------------------+
                    |   Shell (Lab 1)  |
                    |   shell.c        |
                    | Entry point,     |
                    | command dispatch  |
                    +--------+---------+
                             |
                    +--------v---------+
                    | Program Manager  |
                    | program_manager.c|
                    | PID assignment,  |
                    | state tracking   |
                    +--------+---------+
                             |
              +--------------+--------------+
              |                             |
     +--------v---------+         +--------v---------+
     | Parser (Lab 3)   |         | Debugger (Lab 2) |
     | parser.y, lexer.l|         | debugger_vm.c    |
     | Tokenize, parse, |         | Breakpoints,     |
     | build AST with   |         | stepping, inspect|
     | line metadata     |         +--------+---------+
     +--------+---------+                  |
              |                             |
     +--------v---------+                  |
     | Codegen (Lab 3   |                  |
     |  IR role)        |                  |
     | codegen.c        |                  |
     | AST -> bytecode  |                  |
     | + source map     |                  |
     +--------+---------+                  |
              |                             |
     +--------v----------------------------v+
     |        VM (Lab 4)                     |
     |        vm.c                           |
     |        Execute bytecode, step,        |
     |        stack/memory operations        |
     +--------+------------------------------+
              |
     +--------v---------+
     |   GC (Lab 5)     |
     |   gc.c           |
     |   Mark-sweep,    |
     |   heap tracking  |
     +------------------+
```

### Component Interdependencies

- **Shell** depends on **Program Manager** to dispatch commands
- **Program Manager** depends on **Parser** to parse source files
- **Program Manager** depends on **Codegen** to compile ASTs to bytecode
- **Program Manager** depends on **VM** to execute bytecode
- **Program Manager** depends on **GC** for memory statistics and collection
- **Debugger** depends on **VM** for single-step execution
- **Debugger** depends on **Codegen** for source-line-to-PC mapping and variable names
- **VM** depends on **GC** for initialization and cleanup of heap objects

**Removing any single component breaks the entire system.**

---

## File-by-File Breakdown

| File               | Lines | Origin       | Role                                            |
|--------------------|-------|--------------|--------------------------------------------------|
| `main.c`           | 10    | New (Lab 6)  | Entry point: creates ProgramManager, runs shell  |
| `shell.h`          | 14    | New (Lab 6)  | Shell interface declaration                      |
| `shell.c`          | 369   | Lab 1        | Shell loop, tokenizer, pipes, I/O redirect, builtins |
| `ast.h`            | 66    | Lab 3        | AST node types, operator types, constructors     |
| `ast.c`            | 216   | Lab 3        | AST constructors, symbol table, tree-walk evaluator |
| `lexer.l`          | 59    | Lab 3        | Flex tokenizer for `.lang` source files          |
| `parser.y`         | 124   | Lab 3        | Bison grammar rules producing AST nodes          |
| `codegen.h`        | 43    | New (Lab 6)  | Bytecode program structure, source map entries   |
| `codegen.c`        | 240   | New (Lab 6)  | AST-to-bytecode compiler with source-line mapping |
| `instructions.h`   | 36    | Lab 4        | VM opcode definitions (hex constants)            |
| `vm.h`             | 58    | Lab 4 + Lab 5| VM struct with GC fields merged in               |
| `vm.c`             | 460   | Lab 4 + Lab 5| Full instruction executor with GC init/cleanup   |
| `gc.h`             | 72    | Lab 5        | Object types, Value type, GC function declarations |
| `gc.c`             | 169   | Lab 5        | Mark-sweep GC: alloc, mark, sweep, collect       |
| `debugger_vm.h`    | 37    | New (Lab 6)  | Debugger struct and function declarations        |
| `debugger_vm.c`    | 228   | New (Lab 6)  | Interactive debugger: breakpoints, stepping, vars |
| `program_manager.h`| 43    | New (Lab 6)  | Program entry struct, state enum, PM interface   |
| `program_manager.c`| 238   | New (Lab 6)  | Program lifecycle: submit, run, debug, kill, GC  |
| `Makefile`         | 27    | New (Lab 6)  | Build system: bison, flex, gcc                   |

---

## What Was Taken from Each Lab

### Lab 1: Shell and Process Control (`myshell.c` -> `shell.c`)

The **entire Lab 1 shell** was reused as the foundation of `shell.c`. The following were
taken verbatim:

- `Command` struct (argv, infile, outfile, background)
- `sigint_handler()` and `sigchld_handler()` signal handlers
- `trim_sb()` -- string trimming utility
- `tokenize_sb()` -- command-line tokenizer with quote handling
- `free_tokens_sb()` -- token cleanup
- `parse_command_from_tokens_sb()` -- token-to-Command parser with `<`, `>`, `&` handling
- `free_command_sb()` -- command cleanup
- `execute_single_sb()` -- single command execution (cd, exit, fork/exec)
- `execute_pipe_sb()` -- pipe execution with two-process fork
- The main input loop (semicolon splitting, pipe detection, command dispatch)

### Lab 2: Debugger Concepts (`debugger.c` -> `debugger_vm.c`)

Lab 2 was a **ptrace-based native debugger** for ARM64. The integrated system reuses the
**debugger concepts** but reimplements them at the VM level:

- **Breakpoints**: Lab 2 used `PTRACE_POKETEXT` to insert ARM `BRK #0` instructions. The
  integrated debugger sets breakpoints on source lines, mapped to bytecode offsets via the
  source map.
- **Single-stepping**: Lab 2 used `PTRACE_SINGLESTEP`. The integrated debugger uses
  `vm_step()` to execute one bytecode instruction at a time.
- **Register inspection**: Lab 2 used `PTRACE_GETREGSET` to read ARM64 registers. The
  integrated debugger reads `vm->pc`, `vm->sp`, `vm->rsp` directly.
- **Interactive loop**: Lab 2 had a step-continue loop. The integrated debugger provides a
  full interactive command loop (`dbg>` prompt) with break, step, next, continue, vars, etc.

The ptrace code itself was not reused because the integrated system operates at the VM level
rather than the native hardware level. The debugger design (breakpoint/step/inspect pattern)
is preserved.

### Lab 3: Parser, Lexer, and AST (`src/parser.y`, `src/lexer.l`, `src/ast.c/h`)

The **entire Lab 3 parser and AST** were reused:

- `ast.h` / `ast.c`: All node types (`NODE_INT`, `NODE_VAR`, `NODE_OP`, `NODE_ASSIGN`,
  `NODE_DECL`, `NODE_IF`, `NODE_WHILE`, `NODE_SEQ`), all operator types (`OP_ADD` through
  `OP_NEQ`), all constructors (`make_int`, `make_var`, `make_op`, etc.), the symbol table,
  the tree-walk evaluator, and the compatibility wrappers (`createNode`, `createIntNode`).
- `parser.y`: The complete Bison grammar including statement rules, expression rules with
  operator precedence, and all production actions.
- `lexer.l`: The complete Flex lexer with integer, keyword, identifier, and operator tokens.

### Lab 4: Virtual Machine (`vm/vm.c`, `vm/vm.h`, `instructions.h`)

The **entire Lab 4 VM** was reused:

- `vm.h`: The `VM` struct (stack, memory, return_stack, code, pc, sp, rsp, running, error),
  the `VMError` enum, and all function declarations.
- `vm.c`: All instruction implementations -- `OP_PUSH`, `OP_POP`, `OP_DUP`, `OP_ADD`,
  `OP_SUB`, `OP_MUL`, `OP_DIV`, `OP_CMP`, `OP_JMP`, `OP_JZ`, `OP_JNZ`, `OP_STORE`,
  `OP_LOAD`, `OP_CALL`, `OP_RET`, `OP_HALT`. Also `vm_create()`, `vm_destroy()`,
  `vm_load_program()`, `vm_run()`, `vm_dump_state()`, `vm_error_string()`.
- `instructions.h`: All opcode hex constants.

### Lab 5: Garbage Collector (`vm/gc.c`, `vm/gc.h`, `vm/vm.h`)

The **entire Lab 5 GC** was reused:

- `gc.h`: Object types (`OBJ_PAIR`, `OBJ_FUNCTION`, `OBJ_CLOSURE`), the `Object` struct
  with union variants, the `Value` struct and `VAL_INT`/`VAL_OBJ` macros, and all GC
  function declarations.
- `gc.c`: `gc_alloc_object()`, `gc_init()`, `gc_cleanup()`, `new_pair()`, `new_function()`,
  `new_closure()`, `gc_mark_object()` (recursive traversal), `gc_mark_roots()`,
  `gc_sweep()`, `gc_collect()` (with auto-threshold adjustment), `push()`/`pop()` for the
  value stack, and `gc_set_auto_collect()`.
- The GC fields from Lab 5's `vm.h` (`first_object`, `num_objects`, `max_objects`,
  `value_stack`, `stack_count`, `auto_gc`) were merged into the Lab 4 VM struct.

---

## What Was Changed for Integration

### Changes to Lab 1 Code (`shell.c`)

| Change | Detail |
|--------|--------|
| `main()` extracted | The `main()` function was refactored into `shell_run(ProgramManager *pm)` so the shell can receive the program manager from `main.c` |
| `ProgramManager` parameter added | `execute_single_sb()` now takes a `ProgramManager *pm` parameter to dispatch lab6 builtins |
| `handle_lab6_builtin()` added | New function that checks if a command is `submit`, `run`, `debug`, `kill`, `memstat`, `gc`, `leaks`, or `ps` and dispatches to the program manager. Called before Lab 1's original cd/exit/fork-exec path |
| `sigint_handler` simplified | Removed the prompt reprint from the signal handler (the shell loop handles reprompting) |
| `exit` calls `pm_destroy()` | The `exit` builtin now cleans up the program manager before exiting |

All original Lab 1 functionality (fork/exec, pipes, redirection, background, cd, exit,
semicolons, quoting) is **preserved unchanged**.

### Changes to Lab 3 Code (`ast.h`, `ast.c`, `parser.y`, `lexer.l`)

| Change | Detail |
|--------|--------|
| `NODE_PRINT` added to `NodeType` | New AST node type for `print()` statements |
| `line_number` field added to `ASTNode` | Stores source line number for debug metadata (consumed by codegen source map) |
| `make_print()` constructor added | Creates a `NODE_PRINT` node |
| `ast_free()` function added | Recursively frees an AST tree (needed because AST is freed after codegen, not at program exit) |
| `NODE_PRINT` case in `eval()` | Handles print in the tree-walk evaluator |
| `PRINT` token added to parser | New grammar rule: `print_statement: PRINT LPAREN expression RPAREN SEMICOLON` |
| `LE`, `GE` tokens added to parser | Lab 3 only had `EQ`, `NEQ`, `LT`, `GT`; the integrated version adds `<=` and `>=` |
| `%expect 1` added to parser | Suppresses the standard dangling-else shift/reduce conflict warning |
| `line_number` set in grammar actions | Every production action now sets `$$->line_number = yylineno` |
| `%option yylineno` added to lexer | Enables automatic line tracking in Flex |
| `"print"` keyword added to lexer | Recognizes the `print` keyword |
| `"<="` and `">="` tokens added to lexer | Lab 3 had a bug where `<=` and `>=` returned `LT`/`GT`; the integrated version correctly returns `LE`/`GE` |
| `line_number` set in lexer | Integer and identifier tokens now set `yylval->line_number = yylineno` |

### Changes to Lab 4 Code (`vm.h`, `vm.c`, `instructions.h`)

| Change | Detail |
|--------|--------|
| GC fields merged into `VM` struct | `first_object`, `num_objects`, `max_objects`, `value_stack`, `stack_count`, `auto_gc` added from Lab 5's VM struct |
| `vm_create()` allocates `value_stack` | Allocates `VM_STACK_MAX` Value entries for GC root tracking |
| `vm_create()` calls `gc_init()` | Initializes GC state on VM creation |
| `vm_destroy()` calls `gc_cleanup()` | Frees all GC objects before freeing VM memory |
| `vm_step()` function added | Executes a single instruction and returns, used by the debugger for single-stepping |
| `OP_PRINT` (0x50) opcode added | Pops top of stack and prints it; needed for `.lang` print statements |
| `OP_CMP_EQ` through `OP_CMP_GE` added | Five new comparison opcodes (0x15--0x19) for `==`, `!=`, `>`, `<=`, `>=`; Lab 4 only had `OP_CMP` (less-than) |
| `vm_dump_state()` shows GC stats | Prints `num_objects`/`max_objects` in the state dump |
| `vm.h` includes `gc.h` | Needed for `Object` and `Value` type definitions used in the VM struct |

### Changes to Lab 5 Code (`gc.h`, `gc.c`)

**No changes.** The Lab 5 GC was used verbatim. The `gc.h` and `gc.c` files in the
integrated project are identical to the Lab 5 originals.

### New Files for Integration

| File | Purpose |
|------|---------|
| `main.c` | Creates the `ProgramManager`, calls `shell_run()`, cleans up on exit |
| `shell.h` | Header declaring `shell_run(ProgramManager *pm)` |
| `program_manager.h` | Defines `ProgramEntry`, `ProgramState`, `ProgramManager` structs and all PM functions |
| `program_manager.c` | Implements the full program lifecycle: `pm_submit()` (parse + compile), `pm_run()` (VM execution), `pm_debug()` (launch debugger), `pm_kill()`, `pm_memstat()`, `pm_gc()`, `pm_leaks()`, `pm_list()` |
| `codegen.h` | Defines `BytecodeProgram` (code buffer + variable names + source map), and codegen API |
| `codegen.c` | AST-to-bytecode compiler: traverses the AST and emits VM opcodes with source-line mappings. Provides `codegen_line_for_pc()` and `codegen_pc_for_line()` for debugger integration |
| `debugger_vm.h` | Defines `Debugger` struct (VM reference, bytecode program, breakpoints) |
| `debugger_vm.c` | Interactive debugger: breakpoint management, instruction stepping, source-line stepping, continue-to-breakpoint, register/stack/variable/memstat inspection |
| `Makefile` | Build system handling bison, flex, and gcc compilation |

---

## Data Flow Between Components

### `submit <file>` Flow

```
shell.c                    program_manager.c         parser.y / lexer.l     codegen.c
-------                    -----------------         ------------------     ---------
handle_lab6_builtin()  ->  pm_submit(filename)  ->   yyparse()          ->  codegen_compile(root)
                           Opens file, sets yyin      Tokenizes source       Walks AST, emits bytecode
                                                      Builds AST with        Builds source map
                                                      line_number metadata   Records variable names
                                                                         <-  Returns BytecodeProgram
                           Stores PID, filename,
                           state=SUBMITTED,
                           BytecodeProgram in
                           ProgramEntry
```

### `run <pid>` Flow

```
program_manager.c          vm.c
-----------------          ----
pm_run(pid)            ->  vm_create()
 Finds ProgramEntry        Allocates stack, memory, return_stack, value_stack
 Copies bytecode           gc_init() initializes GC
                       ->  vm_load_program(code, size)
                       ->  vm_run()
                            Loops: execute_instruction()
                            Each instruction modifies stack/memory/PC
                       <-  Returns VM_OK or error
 Sets state=FINISHED
 or state=ERROR
```

### `debug <pid>` Flow

```
program_manager.c       debugger_vm.c              vm.c              codegen.c
-----------------       -------------              ----              ---------
pm_debug(pid)       ->  debugger_create(vm, bc)
                    ->  debugger_interactive()
                         Reads user commands:
                         "break 6"              ->                    codegen_pc_for_line(bc, 6)
                                                                     Returns bytecode offset
                         "step"                 ->  vm_step()
                                                    Executes 1 instr
                                                <-  Returns error
                                                ->                    codegen_line_for_pc(bc, pc)
                                                                     Returns source line
                         "vars"                 ->  Reads vm->memory[slot]
                                                ->                    codegen_var_name(bc, slot)
                                                                     Returns "x", "y", etc.
                         "memstat"              ->  Reads vm->num_objects, max_objects
                         "continue"             ->  vm_step() in loop
                                                    Checks breakpoints via source map
                         "quit"                 <-
                    <-  debugger_destroy()
```

### `memstat <pid>` / `gc <pid>` / `leaks <pid>` Flow

```
program_manager.c          gc.c / vm fields
-----------------          ----------------
pm_memstat(pid)        ->  Reads vm->num_objects, vm->max_objects,
                            vm->auto_gc, vm->sp, bc->var_count

pm_gc(pid)             ->  gc_collect(vm)
                            gc_mark_roots() -- marks from value_stack
                            gc_sweep() -- frees unmarked objects

pm_leaks(pid)          ->  Reads vm->num_objects
                            Walks vm->first_object linked list
                            Reports type and marked status
```

---

## Test Programs

Three test programs are provided in the `tests/` directory:

### `tests/hello.lang`

```
var x = 21;
var y = x + 21;
print(y);
```

**Expected output:** `42`

Tests: variable declaration, initialization, arithmetic expressions, print.

### `tests/fibonacci.lang`

```
var a = 0;
var b = 1;
var i = 0;
var temp;
while (i < 10) {
    print(a);
    temp = a + b;
    a = b;
    b = temp;
    i = i + 1;
}
```

**Expected output:**
```
0
1
1
2
3
5
8
13
21
34
```

Tests: while loops, multiple variables, reassignment, comparison operators.

### `tests/ifelse.lang`

```
var x = 10;
var y = 20;
if (x < y) {
    print(x);
} else {
    print(y);
}
var z = x + y;
print(z);
```

**Expected output:**
```
10
30
```

Tests: if/else branching, comparison, blocks, sequential statements.

### Running All Tests

```bash
# Quick test: submit and run all three programs
echo 'submit tests/hello.lang
run 1
submit tests/fibonacci.lang
run 2
submit tests/ifelse.lang
run 3
ps
exit' | ./lab6shell
```

### Testing the Debugger

```bash
echo 'submit tests/fibonacci.lang
debug 1
break 6
continue
vars
next
vars
memstat
quit
exit' | ./lab6shell
```

### Testing Memory Commands

```bash
echo 'submit tests/hello.lang
run 1
memstat 1
gc 1
leaks 1
exit' | ./lab6shell
```

### Testing Shell Features (Lab 1)

```bash
echo 'echo hello world
ls tests
echo foo | cat
exit' | ./lab6shell
```

---

## Example Sessions

### Full Workflow

```
$ ./lab6shell
myshell> submit tests/hello.lang
Program 'tests/hello.lang' submitted as PID 1 (33 bytes bytecode, 2 vars)
myshell> run 1
Running PID 1...
42
PID 1 finished successfully
myshell> memstat 1
=== Memory Stats for PID 1 ===
GC Objects:    0
GC Threshold:  8
Auto GC:       enabled
Stack Depth:   0
Memory Slots:  2 used
myshell> leaks 1
PID 1: No leaks detected (0 objects on heap)
myshell> ps
PID  State       File
---  ----------  ----
1    FINISHED    tests/hello.lang
myshell> exit
```

### Debugging Session

```
$ ./lab6shell
myshell> submit tests/fibonacci.lang
Program 'tests/fibonacci.lang' submitted as PID 1 (120 bytes bytecode, 4 vars)
myshell> debug 1
Debugger ready. Type 'help' for commands.
Program loaded: 120 bytes, 4 variables
dbg> break 6
Breakpoint set at line 6 (pc=25)
dbg> continue
0
Hit breakpoint at line 6 (PC=25)
dbg> vars
Variables:
  a = 0 (slot 0)
  b = 1 (slot 1)
  i = 0 (slot 2)
  temp = 0 (slot 3)
dbg> next
  Stopped at line 7 (PC=56)
dbg> next
  Stopped at line 8 (PC=66)
dbg> vars
Variables:
  a = 0 (slot 0)
  b = 1 (slot 1)
  i = 0 (slot 2)
  temp = 1 (slot 3)
dbg> memstat
GC Objects: 0
GC Threshold: 8
Auto GC: enabled
dbg> continue
1
Hit breakpoint at line 6 (PC=25)
dbg> vars
Variables:
  a = 1 (slot 0)
  b = 1 (slot 1)
  i = 1 (slot 2)
  temp = 1 (slot 3)
dbg> quit
Exiting debugger
myshell> exit
```

### Mixing Shell and Lab Commands

```
$ ./lab6shell
myshell> ls tests/
fibonacci.lang  hello.lang  ifelse.lang
myshell> submit tests/ifelse.lang
Program 'tests/ifelse.lang' submitted as PID 1 (76 bytes bytecode, 3 vars)
myshell> run 1
Running PID 1...
10
30
PID 1 finished successfully
myshell> echo "done testing" | cat
done testing
myshell> gc 1
Forcing GC on PID 1...
[GC] Collected 0 objects, 0 remaining
myshell> kill 1
PID 1 killed
myshell> exit
```
