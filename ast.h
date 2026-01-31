#ifndef AST_H
#define AST_H

/* ===== AST Node Types ===== */
typedef enum {
    NODE_INT,
    NODE_VAR,
    NODE_OP,
    NODE_ASSIGN,
    NODE_DECL,
    NODE_IF,
    NODE_WHILE,
    NODE_SEQ,
    NODE_PRINT    /* LAB6 CHANGE: added for print() statement support */
} NodeType;

/* ===== Operator Types ===== */
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_EQ,
    OP_NEQ
} OpType;

/* ===== AST Node ===== */
typedef struct ASTNode {
    NodeType type;
    int value;
    char *varName;
    int line_number;    /* LAB6 CHANGE: source line for debug metadata */

    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *extra;
    struct ASTNode *next;   /* required by parser */

} ASTNode;

/* ===== Student B Constructors ===== */
ASTNode *make_int(int value);
ASTNode *make_var(char *name);
ASTNode *make_op(OpType op, ASTNode *l, ASTNode *r);
ASTNode *make_assign(char *name, ASTNode *expr);
ASTNode *make_decl(char *name, ASTNode *init);
ASTNode *make_if(ASTNode *cond, ASTNode *then_b, ASTNode *else_b);
ASTNode *make_while(ASTNode *cond, ASTNode *body);
ASTNode *make_seq(ASTNode *first, ASTNode *second);
ASTNode *make_print(ASTNode *expr);    /* LAB6 CHANGE: print constructor */

/* ===== Compatibility Wrappers (DO NOT REMOVE) ===== */
ASTNode *createNode(NodeType type, ASTNode *l, ASTNode *r);
ASTNode *createIntNode(int value);

/* ===== Evaluation ===== */
int eval(ASTNode *node);

/* LAB6 CHANGE: free AST tree for memory cleanup after codegen */
void ast_free(ASTNode *node);

#endif
