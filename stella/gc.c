#include <stdlib.h>
#include <stdio.h>

#include "runtime.h"
#include "gc.h"

/** Total allocated number of bytes (over the entire duration of the program). */
int total_allocated_bytes = 0;

/** Total allocated number of objects (over the entire duration of the program). */
int total_allocated_objects = 0;

int max_allocated_bytes = 0;
int max_allocated_objects = 0;

int cycle_allocated_bytes = 0;
int cycle_allocated_objects = 0;

int total_reads = 0;
int total_writes = 0;

#define MAX_GC_ROOTS 1024

int gc_roots_max_size = 0;
int gc_roots_top = 0;
void **gc_roots[MAX_GC_ROOTS];

// Copying GC
#define MAX_HEAP_SIZE 1600

int gc_cycles = 0;

void *from_space;
void *alloc_pos;

void *to_space;
void *next;
void *scan;

// Utils
#define MAX(a, b) ((a > b) ? a : b )

int field_count(stella_object *obj) {
  return STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
}

int size_of_object(stella_object *obj) {
  return sizeof(stella_object) + (field_count(obj) * sizeof(void*));
}

// Implementation
void init_heap() {
  printf("[GC] Initalizing heap: ");
  from_space = malloc(MAX_HEAP_SIZE);
  to_space = malloc(MAX_HEAP_SIZE);
  alloc_pos = from_space;
  printf("from_space = [%p : %p]; to_space = [%p : %p]\n", from_space, from_space + MAX_HEAP_SIZE, to_space, to_space + MAX_HEAP_SIZE);
}

int points_to(void *space, void *p) {
  return p >= space && p < (space + MAX_HEAP_SIZE);
}

void chase(stella_object *p) {
  do {
    // printf("[CHASE] p = %p\n", p);
    stella_object *q = next;
    next = next + size_of_object(p);
    void *r = NULL;
    // printf("[CHASE] field_count = %d\n", field_count);
    q->object_header = p->object_header;
    for (int i = 0; i < field_count(p); i++) {
      stella_object* fi = p->object_fields[i];
      q->object_fields[i] = fi;
      if (points_to(from_space, fi) && !points_to(to_space, fi->object_fields[0])) {
        r = fi;
      }
    }
    p->object_fields[0] = q;
    p = r;
  } while (p != NULL);
}

void* forward(void *p) {
  // printf("[FORWARD] p = %p\n", p);
  if (points_to(from_space, p)) {
    stella_object *obj = (stella_object*) p;
    if (points_to(to_space, obj->object_fields[0])) {
      return obj->object_fields[0];
    } else {
      chase(obj);
      return obj->object_fields[0];
    }
  } else {
    return p;
  }
}

void gc() {
  gc_cycles++;
  scan = next = to_space;
  printf("[GC] Start GC: scan = %p, next = %p\n", scan, next);
  #ifdef STELLA_DUMP_GC_STATE_ON_GC
  print_gc_state();
  #endif
  for (int i = 0; i < gc_roots_top; i++) {
    void **root = gc_roots[i];
    *root = forward(*root);
  }
  printf("[GC] Finish forwarding roots: scan = %p, next = %p\n", scan, next);
  while (scan < next) {
    stella_object *obj = scan;
    for (int i = 0; i < field_count(obj); i++) {
      obj->object_fields[i] = forward(obj->object_fields[i]);
    }
    scan = scan + size_of_object(obj);
  }
  printf("[GC] Finish forwarding fields: scan = %p, next = %p\n", scan, next);
  void *tmp = to_space;
  to_space = from_space;
  from_space = tmp;
  alloc_pos = next;
  printf("[GC] Finish GC: collected %ld bytes of garbage\n", from_space + MAX_HEAP_SIZE - alloc_pos);
  #ifdef STELLA_DUMP_GC_STATE_ON_GC
  print_gc_state();
  #endif
}

void* gc_alloc(size_t size_in_bytes) {
  if (from_space == NULL || to_space == NULL) {
    init_heap();
  }

  printf("[GC] Start allocation of %zu bytes at %p\n", size_in_bytes, alloc_pos);
  if (alloc_pos + size_in_bytes > (from_space + MAX_HEAP_SIZE)) {
    gc();
    max_allocated_bytes = MAX(max_allocated_bytes, cycle_allocated_bytes);
    max_allocated_objects = MAX(max_allocated_objects, cycle_allocated_objects);
    cycle_allocated_bytes = 0;
    cycle_allocated_objects = 0;
  }

  if (alloc_pos + size_in_bytes > (from_space + MAX_HEAP_SIZE)) {
    printf("[GC] Out of memory\n");
    exit(12);
  }

  void *obj = alloc_pos;
  alloc_pos += size_in_bytes;
  printf("[GC] Finish allocation of %zu bytes at %p\n", size_in_bytes, obj);

  total_allocated_bytes += size_in_bytes;
  total_allocated_objects += 1;
  cycle_allocated_bytes += size_in_bytes;
  cycle_allocated_objects += 1;

  return obj;
}

void print_gc_roots() {
  printf("ROOTS: count = %d\n", gc_roots_top);
  for (int i = 0; i < gc_roots_top; i++) {
    printf("  %p -> %p\n", gc_roots[i], *gc_roots[i]);
  }
}

void print_gc_alloc_stats() {
  printf("Total memory allocation: %'d bytes (%'d objects)\n", total_allocated_bytes, total_allocated_objects);
  printf("Maximum residency:       %'d bytes (%'d objects)\n", MAX(max_allocated_bytes, cycle_allocated_bytes), MAX(max_allocated_objects, cycle_allocated_objects));
  printf("Total memory use:        %'d reads and %'d writes\n", total_reads, total_writes);
  printf("Max GC roots stack size: %'d roots\n", gc_roots_max_size);
  printf("GC cycles:               %'d cycles", gc_cycles);
}

// It's more likely to be a "heap dump" rather than gc state
void print_gc_state() {
  printf("------------------------------------------------------------\n");
  printf("Garbage collector (GC) state:\n");
  
  printf("HEAP: used = %ld bytes; free = %ld bytes\n", alloc_pos - from_space, from_space + MAX_HEAP_SIZE - alloc_pos);
  void *p = from_space;
  while (p < alloc_pos) {
    printf("  %p : ", p);
    print_stella_object(p);
    printf("\n");
    p += size_of_object(p);
  }

  print_gc_roots();

  printf("------------------------------------------------------------\n");
}

void gc_read_barrier(void *object, int field_index) {
  total_reads += 1;
}

void gc_write_barrier(void *object, int field_index, void *contents) {
  total_writes += 1;
}

void gc_push_root(void **ptr){
  gc_roots[gc_roots_top++] = ptr;
  if (gc_roots_top > gc_roots_max_size) { gc_roots_max_size = gc_roots_top; }
}

void gc_pop_root(void **ptr){
  gc_roots_top--;
}
