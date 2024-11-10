# Stella Garbage Collector

Implementation of incremental (Baker's algorithm) copying garbage collector for [Stella](https://fizruk.github.io/stella/) programming language.

Tested on Ubuntu 24.04.

## Executables

* [`stella2c`](stella2c) - converts Stella program into C program
  * Usage: `./stella2c < program.st > program.c`
  * you will have to compile it and run executable file manually
  * you can steal `gcc` command from `build.sh` 
* [`build.sh`](build.sh) - builds Stella program into executable file 
  * Usage: `./build.sh program.st`
  * you will have to run it manually
* [`run.sh`](run.sh) - runs Stella program
  * Usage; `./run.sh program.st <input>` 

Note: both `build.sh` and `run.sh` will create `.c` and executable file in the project root directory.

## Usage example

`square.st`:
```rust
language core;

// addition of natural numbers
fn Nat::add(n : Nat) -> fn(Nat) -> Nat {
  return fn(m : Nat) {
    return Nat::rec(n, m, fn(i : Nat) {
      return fn(r : Nat) {
        return succ( r ) // r := r + 1
      }
    })
  }
}

// square, computed as a sum of odd numbers
fn square(n : Nat) -> Nat {
  return Nat::rec(n, 0, fn(i : Nat) {
      return fn(r : Nat) {
        // r := r + (2*i + 1)
        return Nat::add(i)( Nat::add(i)( succ( r )))
      }
  })
}

fn main(n : Nat) -> Nat {
  return square(n)
}
```

Run:
```bash
./run.sh square.st 5
```

## Available parameters

* `STELLA_DEBUG` - print debug info
* `STELLA_GC_STATS` - print garbage collector (GC) statistics at the end of program execution
* `STELLA_RUNTIME_STATS` - print runtime statistics at the end of program execution
* `STELLA_GC_STATE_ON_GC_START` - print GC state before garbage collection
* `STELLA_GC_STATE_ON_GC_END` - print GC state after garbage collection
* `STELLA_GC_STATE_ON_STATS` - print GC state with GS statistics at the end of program execution
* `STELLA_GC_STATS_ON_OOM` - print GC statistics when program runs out of memory
* `MAX_SPACE_SIZE` - max size of from-space/to-space. It means, that GC will allocate `2 * MAX_SPACE_SIZE` bytes (total heap size). See [source](stella/gc.c)

## Garbage Collector debug info format examples

`GC_STATS`:
```
Garbage collector (GC) statistics:
Total memory allocation: 5032 bytes (298 objects)
Maximum residency:       3192 bytes (192 objects)
Total memory use:        29465 reads and 0 writes
Max GC roots stack size: 31 roots
GC cycles:               2 cycles
Total read forwardings:  200 reads
```

`GC_STATE`:
```
Garbage collector (GC) state:
HEAP: free = 8 bytes, used = 3192 bytes, scan = 0x5631766f5320, next = 0x5631766f5320, limit = 0x5631766f65c8
FROM-SPACE [0x5631766f5fb0 : 0x5631766f6c30] (active):
  0x5631766f5fb0 : 11
  0x5631766f5fc0 : 10
  0x5631766f5fd0 : 9
ROOTS: count = 31
  0x7ffea0a8e968 -> 0x563175dbb040
  0x7ffea0a8e970 -> 0x5631766f5fb0
  0x7ffea0a8e950 -> 0x5631766f5fb0
```
**to-space** is printed only when GC is in incremental mode
```
Garbage collector (GC) state:
HEAP: free = 40 bytes, used = 3160 bytes, scan = 0x5631766f65b0, next = 0x5631766f65b0, limit = 0x5631766f65d8
FROM-SPACE [0x5631766f5320 : 0x5631766f5fa0]:
  0x5631766f5320 : 2
  0x5631766f5330 : 3
  0x5631766f5340 : 4
TO-SPACE [0x55de61a29fb0 : 0x55de61a2ac30] (active):
  0x5631766f5fb0 : 11
  0x5631766f5fc0 : 10
  0x5631766f5fd0 : 9
  ...
  0x5631766f65d8 : 118
  0x5631766f65e8 : fn<0x563175db6418>
  0x5631766f6600 : fn<0x563175db6418>
ROOTS: count = 25
  0x7ffea0a8e968 -> 0x563175dbb040
  0x7ffea0a8e970 -> 0x5631766f5fb0
  0x7ffea0a8e950 -> 0x5631766f5fb0
```

Other debug information is always printed and contains state of variables used in particular part of GC algorithm.