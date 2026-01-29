# Lab 6 Integrated System - Makefile
# Requires: gcc, flex, bison (Linux)

CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lfl

SRCS = main.c shell.c ast.c codegen.c vm.c gc.c debugger_vm.c program_manager.c
GENERATED = lex.yy.c parser.tab.c parser.tab.h

TARGET = lab6shell

all: $(TARGET)

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

$(TARGET): $(SRCS) lex.yy.c parser.tab.c
	$(CC) $(CFLAGS) -o $@ $(SRCS) lex.yy.c parser.tab.c $(LDFLAGS)

clean:
	rm -f $(TARGET) lex.yy.c parser.tab.c parser.tab.h

.PHONY: all clean
