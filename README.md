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
* `STELLA_GC_STATE_ON_STATS` - print GC state with GC statistics at the end of program execution
* `STELLA_STATS_ON_OOM` - print GC and runtime statistics (if enabled) when program runs out of memory (can lead to `Segmentation fault`...)
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
HEAP: free = 8 bytes, used = 3192 bytes, scan = 0x565194c6a320, next = 0x565194c6a320, limit = 0x565194c6b5c8
FROM-SPACE [0x565194c6afb0 : 0x565194c6bc30] (active):
  0x565194c6afb0 : [1] 11
  0x565194c6afc0 : [1] 10
  0x565194c6afd0 : [1] 9
  ...
  0x565194c6b5c8 : [4] fn<0x565193ec032f>
  0x565194c6b5d8 : [1] 118
  0x565194c6b5e8 : [4] fn<0x565193ec0418>
ROOTS: count = 31
  0x7ffdb075ac28 -> 0x565193ec5040
  0x7ffdb075ac30 -> 0x565194c6afb0
  0x7ffdb075ac10 -> 0x565194c6afb0
```
**to-space** is printed only when GC is in incremental mode
```
Garbage collector (GC) state:
HEAP: free = 16 bytes, used = 3184 bytes, scan = 0x55d9229b2670, next = 0x55d9229b2bf0, limit = 0x55d9229b2c00
FROM-SPACE [0x55d9229b2fb0 : 0x55d9229b3c30]:
  0x55d9229b2fb0 : [1] 13
  0x55d9229b2fc0 : [1] 12
  0x55d9229b2fd0 : [1] 11
TO-SPACE [0x55d9229b2320 : 0x55d9229b2fa0] (active):
  0x55d9229b2320 : [1] 12
  0x55d9229b2330 : [1] 11
  0x55d9229b2340 : [1] 10
  ...
  0x55d9229b2c00 : [4] fn<0x55d9213925c9>
  0x55d9229b2c18 : [1] 143
  0x55d9229b2c28 : [4] fn<0x55d921392229>
ROOTS: count = 21
  0x7ffc64c52598 -> 0x55d921397040
  0x7ffc64c525a0 -> 0x55d9229b2320
  0x7ffc64c52580 -> 0x55d9229b2320
```

Other debug information is always printed and contains state of variables used in particular part of GC algorithm.