#define NOB_IMPLEMENTATION

#include "nob.h"
#include <stdlib.h>

typedef uint32_t u32;
typedef uint8_t  u8;

typedef struct {
  char instruction;
  u32 operand;
} Instruction;

typedef struct {
  Instruction *items;
  u32 count;
  u32 capacity;
} Instructions;

typedef struct {
  u32 jz_index;
  u32 jnz_index;
} Jump;

typedef struct {
  Jump *items;
  u32 count;
  u32 capacity;
} Jumps;

#define  Op_INC   '+'
#define  Op_DEC   '-'
#define  Op_LEFT  '<'
#define  Op_RIGHT '>'
#define  Op_JZ    '['
#define  Op_JNZ   ']'
#define  Op_OUT   '.'
#define  Op_IN    ','

void parse_source(Nob_String_View *source, Instructions *instructions, Jumps *jumps) {
  Jumps jumps_temp = {0};

  for (int i=0; i<source->count; ++i) {
    char current_char = source->items[i];
    switch (current_char) {
      case Op_INC: {
        Instruction ins = {
          .instruction=current_char,
          .operand=1,
        };
        da_append(instructions, ins);
      }
      break;
      case Op_DEC: {
        Instruction ins = {
          .instruction=current_char,
          .operand=1,
        };
        da_append(instructions, ins);
      }
      break;
      case Op_LEFT: {
        Instruction ins = {
          .instruction=current_char,
          .operand=1,
        };
        da_append(instructions, ins);
      }
      break;
      case Op_RIGHT: {
        Instruction ins = {
          .instruction=current_char,
          .operand=1,
        };
        da_append(instructions, ins);
      }
      break;
      case Op_JZ: {
        Instruction ins = {
          .instruction=current_char,
          .operand=1,
        };

        Jump jump = { .jz_index=instructions->count, .jnz_index=0 };
        da_append(&jumps_temp, jump);
        nob_log(INFO, "Insert jz jump at %d", jump.jz_index);

        da_append(instructions, ins);
      }
      break;
      case Op_JNZ: {
        Jump jump = da_pop(&jumps_temp);
        size_t jz_index = jump.jz_index;

        // Back patch JZ instruction to jump here.
        instructions->items[jz_index].operand = instructions->count;

        Instruction ins = {
          .instruction=current_char,
          .operand=jz_index,
        };
        nob_log(INFO, "Patch jz jump at %zu with destination %d", jz_index, instructions->count);

        da_append(instructions, ins);
      }
      break;
      case Op_OUT: {
        Instruction ins = {
          .instruction=current_char,
          .operand=1,
        };
        da_append(instructions, ins);
      }
      break;
      case Op_IN: {
        Instruction ins = {
          .instruction=current_char,
          .operand=1,
        };
        da_append(instructions, ins);
      }
      break;
    };
  }
}


void execute_code(Instructions *instructions) {
  u32 ip=0;
  size_t head=0;
  u8 mem[1024];
  memset(mem, 0, sizeof(mem));

  while (ip < instructions->count) {
    Instruction op = instructions->items[ip];
    switch (op.instruction) {
      case Op_INC: {
        mem[head]++;
      }
      break;
      case Op_DEC: {
        mem[head]--;
      }
      break;
      case Op_LEFT: {
        head--;
      }
      break;
      case Op_RIGHT: {
        head++;
      }
      break;
      case Op_JZ: {
        if (mem[head] == 0)
          ip=op.operand;
      }
      break;
      case Op_JNZ: {
        if (mem[head] != 0)
          ip=op.operand;
      }
      break;
      case Op_OUT: {
        printf("%c", mem[head]);
      }
      break;
      case Op_IN: {
        char c;
        ssize_t re = read(0 /* stdin */, &c, 1);
        if (re == 0) {
          c = 0;
        } else if (re < 0) {
          fprintf(stderr, "ERROR: %s\n", strerror(errno));
          exit(42);
        }
        mem[head]=c;
      }
      break;
    };
    ip++;
  }
}


int main(int argc, char **argv) {
  char* program = nob_shift_args(&argc, &argv);
  if (argc == 0) {
    printf("Usage: %s <brainfuck source filename>\n", program);
    return 1;
  }

  // Disable log if DEBUG is not set
  const char* DEBUG = getenv("DEBUG");
  if (DEBUG == NULL || strcmp("0", DEBUG) == 0 || strcmp(DEBUG, "false") == 0) {
    nob_set_log_handler(nob_null_log_handler);
  }

  char* filename = nob_shift_args(&argc, &argv);
  nob_log(INFO, "filename: %s", filename);

  Nob_String_Builder source = {0};
  if (!nob_read_entire_file(filename, &source)) {
    fprintf(stderr, "File not found: %s\n", filename);
    return 1;
  }
  Nob_String_View source_sv = nob_sb_to_sv(source);
  Instructions instructions = {0};
  Jumps jumps = {0};
  parse_source(&source_sv, &instructions, &jumps);

  nob_da_foreach(Instruction, x, &instructions) {
      // `x` here is a pointer to the current element. You can get its index by taking a difference
      // between `x` and the start of the array which is `x.items`.
      size_t index = x - instructions.items;
      nob_log(INFO, "%zu: %c(%d)", index, x->instruction, x->operand);
  }

  execute_code(&instructions);
  return 0;
}
