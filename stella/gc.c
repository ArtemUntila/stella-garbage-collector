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
#define MAX_SPACE_SIZE 3200

int gc_cycles = 0;

void *from_space;
void *alloc_pos;

void *to_space;
void *next;
void *scan;

// Incremental GC
void *limit;
int inc_mode;

int total_read_forwards;

// Utils
#define MAX(a, b) ((a > b) ? a : b )

int field_count(stella_object *obj) {
  return STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
}

size_t size_of_object(stella_object *obj) {
  return sizeof(stella_object) + (field_count(obj) * sizeof(void*));
}

void print_space_range(void *space) {
  printf("[%p : %p]", space, space + MAX_SPACE_SIZE);
}

// Implementation
void init_heap() {
  printf("[GC] Initializing heap: ");
  from_space = malloc(MAX_SPACE_SIZE);
  to_space = malloc(MAX_SPACE_SIZE);
  alloc_pos = from_space;
  limit = from_space + MAX_SPACE_SIZE;
  printf("from_space = ");print_space_range(from_space);printf("; to_space =");print_space_range(to_space);printf("\n");
}

int points_to(void *space, void *p) {
  return p >= space && p < (space + MAX_SPACE_SIZE);
}

void check_oom(size_t size_in_bytes) {
  if (alloc_pos + size_in_bytes > limit) {
    printf("Out of memory\n");
    #ifdef STELLA_STATS_ON_OOM
    print_stella_stats();
    #endif
    exit(12);
  }
}

void chase(stella_object *p) {
  do {
    stella_object *q = next;
    size_t size_of_p = size_of_object(p);
    check_oom(size_of_p);
    next = next + size_of_p;
    alloc_pos = next; // sync incremental
    void *r = NULL;
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
  printf("[GC] Start GC\n");
  #ifdef STELLA_GC_STATE_ON_GC_START
  print_gc_state();
  #endif
  limit = to_space + MAX_SPACE_SIZE;
  alloc_pos = to_space;
  for (int i = 0; i < gc_roots_top; i++) {
    void **root = gc_roots[i];
    *root = forward(*root);
  }
  printf("[GC] Finish forwarding roots\n");
  inc_mode = 1;
}

void inc_gc() {
  if (scan < next) {
    printf("[GC] Incrementally forwarding fields of object at %p\n", scan);
    stella_object *obj = scan;
    for (int i = 0; i < field_count(obj); i++) {
      obj->object_fields[i] = forward(obj->object_fields[i]);
    }
    scan = scan + size_of_object(obj);
  }
  if (scan == next) {
    printf("[GC] Finish incremental GC\n");
    inc_mode = 0;
    void *tmp = to_space;
    to_space = from_space;
    from_space = tmp;
    #ifdef STELLA_GC_STATE_ON_GC_END
    print_gc_state();
    #endif
  }
}

void* gc_alloc(size_t size_in_bytes) {
  if (from_space == NULL || to_space == NULL) {
    init_heap();
  }

  printf("[GC] Allocating %zu bytes\n", size_in_bytes);
  if (!inc_mode && alloc_pos + size_in_bytes > limit) {
    gc();
    max_allocated_bytes = MAX(max_allocated_bytes, cycle_allocated_bytes);
    max_allocated_objects = MAX(max_allocated_objects, cycle_allocated_objects);
    cycle_allocated_bytes = 0;
    cycle_allocated_objects = 0;
  }

  check_oom(size_in_bytes);

  void *obj;
  if (inc_mode) {
    inc_gc();
    check_oom(size_in_bytes);
    limit -= size_in_bytes;
    obj = limit;
  } else {
    obj = alloc_pos;
    alloc_pos += size_in_bytes;
  }
  printf("[GC] Allocated %zu bytes at %p\n", size_in_bytes, obj);

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
  #ifdef STELLA_GC_STATE_ON_STATS
  print_gc_state();
  #endif
  printf("Total memory allocation: %'d bytes (%'d objects)\n", total_allocated_bytes, total_allocated_objects);
  printf("Maximum residency:       %'d bytes (%'d objects)\n", MAX(max_allocated_bytes, cycle_allocated_bytes), MAX(max_allocated_objects, cycle_allocated_objects));
  printf("Total memory use:        %'d reads and %'d writes\n", total_reads, total_writes);
  printf("Max GC roots stack size: %'d roots\n", gc_roots_max_size);
  printf("GC cycles:               %'d cycles\n", gc_cycles);
  printf("Total read forwardings:  %'d reads\n", total_read_forwards);
}

void print_stella_object_with_header(stella_object *obj) {
    printf("[%d] ", STELLA_OBJECT_HEADER_TAG(obj->object_header));print_stella_object(obj);
}

void print_mem(void *start, void *end) {
  void *p = start;
  while (p < end) {
    printf("  %p : ", p);print_stella_object_with_header(p);printf("\n");
    p += size_of_object(p);
  }
}

void print_active_space(void *space) {
  print_mem(space, alloc_pos);
  if (limit < space + MAX_SPACE_SIZE) {
    printf("  ...\n");
    print_mem(limit, space + MAX_SPACE_SIZE);
  }
}

void print_from_space() {
  printf("FROM-SPACE");print_space_range(from_space);
  if (points_to(from_space, alloc_pos)) {
    printf(" (active):\n");
    print_active_space(from_space);
  } else {
    printf(":\n");
    print_mem(from_space, from_space + MAX_SPACE_SIZE);
  }
}

void print_to_space() {
  if (!points_to(to_space, alloc_pos)) return;
  printf("TO-SPACE ");print_space_range(to_space);printf(" (active):\n");
  print_active_space(to_space);
}

void print_gc_state() {
  printf("------------------------------------------------------------\n");
  printf("Garbage collector (GC) state:\n");
  printf("HEAP: free = %ld bytes, used = %ld bytes, scan = %p, next = %p, limit = %p\n", limit - alloc_pos, (MAX_SPACE_SIZE - (limit - alloc_pos)), scan, next, limit);
  print_from_space();
  print_to_space();
  print_gc_roots();
  printf("------------------------------------------------------------\n");
}

void gc_read_barrier(void *object, int field_index) {
  total_reads += 1;
  if (inc_mode) {
    stella_object *obj = (stella_object*) object;
    if (points_to(from_space, obj->object_fields[field_index])) {
      total_read_forwards++;
      obj->object_fields[field_index] = forward(obj->object_fields[field_index]);
    }
  }
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
