---
runme:
  id: 01HWKEC998159S8A296DYGHPR1
  version: v3
---

# brainfuck


An interpreter for [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck) programs.

This implementation uses one byte per cell, interpreted like a C one-byte unsigned integer: increments and decrements are performed modulo 256 (by 'wrapping-around'). Moving the pointer past one end of the tape makes it wrap around to the other side.

Basic usage:
```bash
$ brainfuck FILE -n CELLS
```

More information can be found with `brainfuck --help`.
