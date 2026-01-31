#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
ASTNode *root = NULL;

/* ===== Symbol Table ===== */
#define MAX_VARS 128

typedef struct {
    char name[32];
    int value;
} Symbol;

static Symbol symtab[MAX_VARS];
static int symcount = 0;

static int lookup(char *name) {
    for (int i = 0; i < symcount; i++)
        if (strcmp(symtab[i].name, name) == 0)
            return i;
    return -1;
}

static void declare_var(char *name, int value) {
    if (lookup(name) != -1) return;
    strcpy(symtab[symcount].name, name);
    symtab[symcount].value = value;
    symcount++;
}

static int get_var(char *name) {
    int i = lookup(name);
    if (i == -1) {
        printf("Runtime Error: variable '%s' used before declaration\n", name);
        exit(1);
    }
    return symtab[i].value;
}

static void set_var(char *name, int value) {
    int i = lookup(name);
    if (i == -1) {
        printf("Runtime Error: variable '%s' used before declaration\n", name);
        exit(1);
    }
    symtab[i].value = value;
}

/* ===== Constructors ===== */
ASTNode *make_int(int value) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_INT;
    n->value = value;
    return n;
}

ASTNode *make_var(char *name) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_VAR;
    n->varName = name;
    return n;
}

ASTNode *make_op(OpType op, ASTNode *l, ASTNode *r) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_OP;
    n->value = op;
    n->left = l;
    n->right = r;
    return n;
}

ASTNode *make_assign(char *name, ASTNode *expr) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_ASSIGN;
    n->varName = name;
    n->left = expr;
    return n;
}

ASTNode *make_decl(char *name, ASTNode *init) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_DECL;
    n->varName = name;
    n->left = init;
    return n;
}

ASTNode *make_if(ASTNode *cond, ASTNode *then_b, ASTNode *else_b) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_IF;
    n->left = cond;
    n->right = then_b;
    n->extra = else_b;
    return n;
}

ASTNode *make_while(ASTNode *cond, ASTNode *body) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_WHILE;
    n->left = cond;
    n->right = body;
    return n;
}

ASTNode *make_seq(ASTNode *first, ASTNode *second) {
    if (!first) return second;
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_SEQ;
    n->left = first;
    n->right = second;
    return n;
}

/* LAB6 CHANGE: print statement constructor */
ASTNode *make_print(ASTNode *expr) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = NODE_PRINT;
    n->left = expr;
    return n;
}

/* ===== Compatibility Wrappers ===== */
ASTNode *createIntNode(int value) {
    return make_int(value);
}

ASTNode *createNode(NodeType type, ASTNode *l, ASTNode *r) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = type;
    n->left = l;
    n->right = r;
    return n;
}

/* ===== Evaluation ===== */
int eval(ASTNode *n) {
    if (!n) return 0;

    switch (n->type) {

    case NODE_INT:
        return n->value;

    case NODE_VAR:
        return get_var(n->varName);

    case NODE_OP: {
        int l = eval(n->left);
        int r = eval(n->right);
        switch (n->value) {
            case OP_ADD: return l + r;
            case OP_SUB: return l - r;
            case OP_MUL: return l * r;
            case OP_DIV:
                if (r == 0) {
                    printf("Runtime Error: division by zero\n");
                    exit(1);
                }
                return l / r;
            case OP_LT:  return l < r;
            case OP_GT:  return l > r;
            case OP_LE:  return l <= r;
            case OP_GE:  return l >= r;
            case OP_EQ:  return l == r;
            case OP_NEQ: return l != r;
        }
        break;
    }

    case NODE_DECL: {
        int v = n->left ? eval(n->left) : 0;
        declare_var(n->varName, v);
        return 0;
    }

    case NODE_ASSIGN:
        set_var(n->varName, eval(n->left));
        return 0;

    case NODE_IF:
        if (eval(n->left))
            eval(n->right);
        else if (n->extra)
            eval(n->extra);
        return 0;

    case NODE_WHILE:
        while (eval(n->left))
            eval(n->right);
        return 0;

    case NODE_SEQ:
        eval(n->left);
        eval(n->right);
        return 0;

    /* LAB6 CHANGE: handle print in tree-walk evaluator */
    case NODE_PRINT:
        printf("%d\n", eval(n->left));
        return 0;
    }

    return 0;
}

/* LAB6 CHANGE: free AST tree after codegen */
void ast_free(ASTNode *node) {
    if (!node) return;
    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->extra);
    if (node->varName) free(node->varName);
    free(node);
}
