#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#define OP_PUSH  0x01
#define OP_POP   0x02
#define OP_DUP   0x03

#define OP_ADD   0x10
#define OP_SUB   0x11
#define OP_MUL   0x12
#define OP_DIV   0x13
#define OP_CMP   0x14

/* LAB6 CHANGE: additional comparison opcodes for codegen */
#define OP_CMP_EQ  0x15
#define OP_CMP_NE  0x16
#define OP_CMP_GT  0x17
#define OP_CMP_LE  0x18
#define OP_CMP_GE  0x19

#define OP_JMP   0x20
#define OP_JZ    0x21
#define OP_JNZ   0x22

#define OP_STORE 0x30
#define OP_LOAD  0x31

#define OP_CALL  0x40
#define OP_RET   0x41

/* LAB6 CHANGE: print opcode for output support */
#define OP_PRINT 0x50

#define OP_HALT  0xFF

#endif
