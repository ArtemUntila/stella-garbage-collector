# Stella Garbage Collector

Implementation of copying garbage collector for [Stella](https://fizruk.github.io/stella/) programming language.

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

Note: both `build.sh` and `run.sh` require program to be located in project root dir. In other cases, use `stella2c` and `gcc` manually.

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
* `STELLA_DUMP_GC_STATE_ON_GC` - print GC state (heapdump basically) before and after garbage collection. Note: the behavior of this feature may be undefined, please disable it if you encounter so beloved `Segmentation fault` during heapdump output
* `MAX_HEAP_SIZE` - max size of from-space/to-space. It means, that GC will allocate `2 * MAX_HEAP_SIZE` bytes. See [source](stella/gc.c)

## Garbage Collector debug info format examples

`GC_STATS`:
```
Garbage collector (GC) statistics:
Total memory allocation: 2312 bytes (134 objects)
Maximum residency:       1600 bytes (94 objects)
Total memory use:        3950 reads and 0 writes
Max GC roots stack size: 31 roots
GC cycles:               1 cycles
```

`GC_STATE`:
```
Garbage collector (GC) state:
HEAP: used = 752 bytes; free = 848 bytes
  0x55efa7a8a320 : 7
  0x55efa7a8a330 : 6
  0x55efa7a8a340 : 5
  ...
ROOTS: count = 31
  0x7fff2a0363f8 -> 0x55efa7114040
  0x7fff2a036400 -> 0x55efa7a8a320
  0x7fff2a0363e0 -> 0x55efa7a8a320
  ...
```

Other debug information is always printed and contains state of variables used in particular part of GC algorithm.