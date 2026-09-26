# bf — a Brainfuck interpreter in C

A small, single-file Brainfuck interpreter built on top of
[nob.h](https://github.com/tsoding/nob.h) (a public-domain, header-only C
utility library) for dynamic arrays, string builders, and file I/O.

```
bf.c      the entire interpreter
nob.h     vendored dependency (v3.10.0), not written for this project
Makefile  two build targets
```

## Build

```sh
make          # cc -o bf bf.c
make debug    # cc -g -o bf bf.c
```

Requires a C99 compiler and a POSIX system (`read`, `getenv`). No `-lm`, no
other dependencies.

## Usage

```sh
./bf <brainfuck_source_file>
```

Anything that is not one of the eight opcodes is treated as a comment, so
newlines, spaces, and human-readable labels in the source are ignored.

```sh
$ printf '+++++[>+++++++++++++<-]>.' > /tmp/a.bf   # 5 * 13 = 65 = 'A'
$ ./bf /tmp/a.bf
A
```

## Opcodes

| Char | Name    | Behavior                                          |
| ---- | ------- | ------------------------------------------------- |
| `+`  | INC     | increment the cell under the data pointer          |
| `-`  | DEC     | decrement the cell under the data pointer          |
| `>`  | RIGHT   | move the data pointer right                        |
| `<`  | LEFT    | move the data pointer left                         |
| `[`  | JZ      | if the current cell is 0, jump past the matching `]` |
| `]`  | JNZ     | if the current cell is non-zero, jump back to the matching `[` |
| `.`  | OUT     | write the current cell to stdout as a byte         |
| `,`  | IN      | read one byte from stdin into the current cell     |

The machine has a **1024-cell tape** of zeroed `uint8_t` bytes and a data
pointer starting at cell 0.

## How it works

`main` reads the whole file, then hands off to two phases:

1. **`parse_source`** (`bf.c:40`) — one pass over the source builds a flat
   `Instructions` array of `{char instruction; u32 operand;}` records. Loop
   brackets are the only opcodes that need real work: each `[` pushes its
   index onto a temporary `Jumps` stack, and each `]` pops that stack to learn
   the matching `[`. Once parsing finishes, a second loop back-patches both
   operands so the `[` and `]` each hold the other's instruction index. This is
   the standard "precompute jump targets" technique, so the inner interpreter
   loop never has to search for a matching bracket.

2. **`execute_code`** (`bf.c:136`) — a straightforward dispatch loop. `ip` is
   the instruction pointer; each iteration copies one `Instruction` and
   switches on it, then increments `ip`. `jz`/`jnz` overwrite `ip` with the
   precomputed operand, which combined with the unconditional `ip++` at the
   bottom of the loop produces the "skip past" and "jump back" semantics.

Non-jump opcodes leave `operand` at `1` and it is ignored.

## Debugging

Set `DEBUG` to see the parse, the resolved jump table, and a disassembly:

```sh
$ DEBUG=1 ./bf prog.bf
[INFO] filename: prog.bf
[INFO] Insert jz jump at 3
[INFO] Insert jnz jump: 9
[INFO] Jumps: 0: 3 -> 9
[INFO]  - Jumps: [(9)
[INFO] 0: +(1)
[INFO] 3: [(9)
...
[INFO] 9: ](3)
```

`DEBUG=0` or `DEBUG=false` (or leaving it unset) silences all output. The
log handler is swapped for `nob_null_log_handler` rather than filtering, so
there is no formatting cost in the normal path.

## Verified behavior

Each opcode was checked against a hand-computed expected byte. The program
output is captured straight from stdout, byte for byte, with no post-processing:

| Case    | Program           | Expected | Result |
| ------- | ----------------- | -------- | ------ |
| INC     | `+.`              | `01`     | pass   |
| DEC     | `-.`              | `ff`     | pass   |
| RIGHT   | `>+++.`           | `03`     | pass   |
| LEFT    | `>+++>++.<<-.`    | `02ff`   | pass   |
| loop    | `+++[>+++<-]>.`   | `09`     | pass   |
| IN      | `,.` with `A`     | `41`     | pass   |

`DEC` wrapping to `ff` confirms cells are unsigned 8-bit. End-to-end, an
independently generated Hello World program produces exactly the 12 bytes
`Hello World!`, byte-identical to the expected string.

Note that `+++++.` prints byte `0x05`, not the character `5` — you need 53
increments (or a loop) to print a digit.

## Exit status and errors

| Situation | stdout | stderr | Exit |
| --- | --- | --- | --- |
| Normal run | program output | — | `0` |
| No argument | usage line | — | `1` |
| Source file missing or unreadable | — | `File not found: <path>` | `1` |
| `read` on stdin fails mid-run | — | `ERROR: <strerror>` | `42` |
| Unbalanced `]` | — | `NOB_ASSERT` message | `134` (`SIGABRT`) |

End-of-input is not an error: `,` at EOF stores `0` and execution continues.

One rough edge: the "File not found" wording is used for *any*
`nob_read_entire_file` failure, so a file that exists but is unreadable
(reports as `Permission denied`) is also announced as "File not found".

## Known limitations

These are all reproducible with the current source:

- **No tape bounds checking** (the main one left). The pointer is a `size_t`
  and every access is unguarded, so:
  - `<` at cell 0 underflows to `SIZE_MAX` and reads far outside the array. It
    usually returns garbage silently and still exits `0`.
  - `>` past cell 1023 writes out of bounds. Confirmed: head `1023` is fine,
    head `1024` dies with `SIGABRT`.
- **Unbalanced `]`** trips the `NOB_ASSERT` inside `nob_da_pop` (`bf.c:97`) and
  aborts with a raw assertion message rather than a diagnostic.
  Because that guard is `assert`, it disappears under `-DNDEBUG` — the pop then
  reads before the start of the array and the process dies with `SIGBUS`
  (exit `138`) instead. Note that neither `make` target defines `NDEBUG`, so
  both of them have assertions enabled; `make debug` only adds `-g`.
- **Unbalanced `[` is silently accepted** — no error, exit `0`.
- Cells do not wrap at the tape ends (no wrap-around mode), and the tape size
  is a hard-coded `1024` at `bf.c:139` with no way to configure it.
- This file was generated with opencode free. The code was written by hand.

## License

`bf.c` and `Makefile` are part of this project. `nob.h` is public domain
(© Tsoding), v3.10.0.
