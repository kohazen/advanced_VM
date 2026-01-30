#ifndef GC_H
#define GC_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations - struct keyword required to avoid double typedef */
struct VM;

typedef enum {
    OBJ_PAIR,
    OBJ_FUNCTION,
    OBJ_CLOSURE
} ObjectType;

typedef struct Object {
    bool marked;
    ObjectType type;
    struct Object *next;

    union {
        struct {
            struct Object *left;
            struct Object *right;
        } pair;

        struct {
            void *function_ptr;
        } function;

        struct {
            struct Object *fn;
            struct Object *env;
        } closure;
    };
} Object;

typedef enum {
    VAL_INT,
    VAL_OBJ
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int32_t int_val;
        Object *obj_val;
    };
} Value;

#define VAL_OBJ(obj) ((Value){.type = VAL_OBJ, .obj_val = (obj)})
#define VAL_INT(val) ((Value){.type = VAL_INT, .int_val = (val)})

/* GC functions - use struct VM* to avoid typedef issues */
Object* gc_alloc_object(struct VM *vm, ObjectType type);
void gc_init(struct VM *vm);
void gc_cleanup(struct VM *vm);
Object* new_pair(struct VM *vm, Object *left, Object *right);
Object* new_function(struct VM *vm);
Object* new_closure(struct VM *vm, Object *fn, Object *env);
void gc_mark_object(Object *obj);
void gc_mark_roots(struct VM *vm);
void gc_sweep(struct VM *vm);
void gc_collect(struct VM *vm);
void push(struct VM *vm, Value val);
Value pop(struct VM *vm);
void gc(struct VM *vm);

/* Control automatic GC triggering */
void gc_set_auto_collect(struct VM *vm, bool enabled);

#endif
