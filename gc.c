/* Complete GC implementation with closure support */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"  /* Includes gc.h automatically */

Object* gc_alloc_object(VM *vm, ObjectType type) {
    /* Trigger GC if threshold reached and auto_gc is enabled */
    if (vm->auto_gc && vm->num_objects >= vm->max_objects) {
        gc_collect(vm);
    }

    Object *obj = (Object*)malloc(sizeof(Object));
    if (!obj) {
        fprintf(stderr, "Error: Failed to allocate object\n");
        return NULL;
    }

    obj->marked = false;
    obj->type = type;

    switch (type) {
        case OBJ_PAIR:
            obj->pair.left = NULL;
            obj->pair.right = NULL;
            break;
        case OBJ_FUNCTION:
            obj->function.function_ptr = NULL;
            break;
        case OBJ_CLOSURE:
            obj->closure.fn = NULL;
            obj->closure.env = NULL;
            break;
    }

    obj->next = vm->first_object;
    vm->first_object = obj;
    vm->num_objects++;

    return obj;
}

void gc_init(VM *vm) {
    vm->first_object = NULL;
    vm->num_objects = 0;
    vm->max_objects = 8;
    vm->stack_count = 0;
    vm->auto_gc = true;  /* Enable automatic GC by default */
}

void gc_cleanup(VM *vm) {
    Object *obj = vm->first_object;
    while (obj) {
        Object *next = obj->next;
        free(obj);
        obj = next;
    }
    vm->first_object = NULL;
    vm->num_objects = 0;
}

Object* new_pair(VM *vm, Object *left, Object *right) {
    Object *pair = gc_alloc_object(vm, OBJ_PAIR);
    if (!pair) return NULL;
    pair->pair.left = left;
    pair->pair.right = right;
    return pair;
}

/* Create function object */
Object* new_function(VM *vm) {
    Object *fn = gc_alloc_object(vm, OBJ_FUNCTION);
    return fn;
}

/* Create closure object */
Object* new_closure(VM *vm, Object *fn, Object *env) {
    Object *closure = gc_alloc_object(vm, OBJ_CLOSURE);
    if (!closure) return NULL;
    closure->closure.fn = fn;
    closure->closure.env = env;
    return closure;
}

void gc_mark_object(Object *obj) {
    if (obj == NULL) return;
    if (obj->marked) return;

    obj->marked = true;

    switch (obj->type) {
        case OBJ_PAIR:
            gc_mark_object(obj->pair.left);
            gc_mark_object(obj->pair.right);
            break;
        case OBJ_CLOSURE:
            gc_mark_object(obj->closure.fn);
            gc_mark_object(obj->closure.env);
            break;
        case OBJ_FUNCTION:
            break;
    }
}

void gc_mark_roots(VM *vm) {
    for (int i = 0; i < vm->stack_count; i++) {
        Value *val = &vm->value_stack[i];
        if (val->type == VAL_OBJ) {
            gc_mark_object(val->obj_val);
        }
    }
}

void gc_sweep(VM *vm) {
    Object **obj_ptr = &vm->first_object;

    while (*obj_ptr) {
        if (!(*obj_ptr)->marked) {
            Object *unreached = *obj_ptr;
            *obj_ptr = unreached->next;
            free(unreached);
            vm->num_objects--;
        } else {
            (*obj_ptr)->marked = false;
            obj_ptr = &(*obj_ptr)->next;
        }
    }
}

void gc_collect(VM *vm) {
    int before_count = vm->num_objects;

    gc_mark_roots(vm);
    gc_sweep(vm);

    vm->max_objects = vm->num_objects * 2;
    if (vm->max_objects < 8) {
        vm->max_objects = 8;
    }

    printf("[GC] Collected %d objects, %d remaining\n",
           before_count - vm->num_objects, vm->num_objects);
}

void push(VM *vm, Value val) {
    if (vm->stack_count >= VM_STACK_MAX) {
        fprintf(stderr, "Error: Stack overflow\n");
        return;
    }
    vm->value_stack[vm->stack_count++] = val;
}

Value pop(VM *vm) {
    if (vm->stack_count <= 0) {
        fprintf(stderr, "Error: Stack underflow\n");
        return VAL_INT(0);
    }
    return vm->value_stack[--vm->stack_count];
}

void gc(VM *vm) {
    gc_collect(vm);
}

/* Enable or disable automatic GC triggering */
void gc_set_auto_collect(VM *vm, bool enabled) {
    vm->auto_gc = enabled;
}
