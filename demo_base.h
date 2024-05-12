#ifndef DEMO_BASE_H
#define DEMO_BASE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#define global static
#define function static
#define local_persist static

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef float F32;
typedef double F64;
typedef S32 B32;

#define FALSE 0
#define TRUE 1

// -
// Memory
#define KILOBYTE 1024
#define PAGE (4 * KILOBYTE)
#define MEGABYTE (1024 * 1024)
#define GIGABYTE (1024 * 1024 * 1024)
#define MACHINE_ALIGNMENT (sizeof(void *))

#define KERNEL_SET_ARENA_MEMORY_ADDRESS NULL
#define MMAP_NO_FILE -1
#define MMAP_NO_FILE_OFFSET 0
#define MEMORY_PROTECTION (PROT_READ | PROT_WRITE)
#define MEMORY_RESERVE_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)

#define CHUNK_SIZE PAGE 
#define ARENA_INIT_SIZE (1 * GIGABYTE)

typedef struct Arena Arena;
struct Arena {
  U8 *memory;
  U64 size;
  U64 position;
};

function void arena_init(Arena *arena);
function void *arena_allocate(Arena *arena, U64 size);
function void arena_free(Arena *arena);
function void arena_destroy(Arena *arena);

// -
// ByteArray
typedef struct ByteArray ByteArray;
struct ByteArray
{
  U8 *bytes;
  U64 size;
};

// -
// String
typedef struct String String;
struct String {
  U8 *str;
  U64 size;
};

function String *bl_string_from_cstr(Arena *arena, char *cstr);
function String *bl_string_from_byte_array_slice(Arena *arena, ByteArray *input, U64 start, U64 end);
function char *bl_cstr_from_string(Arena *arena, String *string);
function B32 bl_string_compare(String *first, String *second);

// -
// Convert
#define S32_MAX 2147483647
#define S32_MIN -2147483648

#define MAX_S32_STRING_SIZE 12
#define MAX_B32_STRING_SIZE 6
#define MAX_U32_STRING_SIZE 11
#define MAX_U64_STRING_SIZE 21

typedef struct U8Result U8Result;
struct U8Result
{
  U8 value;
  B32 ok;
};

typedef struct StringResult StringResult;
struct StringResult
{
  String *value;
  B32 ok;
};

typedef struct S32Result S32Result;
struct S32Result
{
  S32 value;
  B32 ok;
};

typedef struct U64Result U64Result;
struct U64Result
{
  U64 value;
  B32 ok;
};

typedef struct F64Result F64Result;
struct F64Result
{
  F64 value;
  B32 ok;
};

typedef struct B32Result B32Result;
struct B32Result
{
  B32 value;
  B32 ok;
};

function B32 bl_is_valid_signed_string_number(String *input);
function B32 bl_is_valid_unsigned_string_number(String *input);
function S32Result bl_s32_from_string(String *input);
function U64Result bl_u64_from_string(String *input);
function B32Result bl_b32_from_string(Arena *arena, String *input);
function F64Result bl_f64_from_string(String *input);
function StringResult *bl_string_from_u64(Arena *arena, U64 value);

// -
#ifdef DEMO_BASE_IMPLEMENTATION

// -
// Arena
function void arena_init(Arena *arena)
{

  // -
  errno = 0;
  U8 *memory = mmap(
      KERNEL_SET_ARENA_MEMORY_ADDRESS,
      ARENA_INIT_SIZE,
      PROT_NONE,
      MEMORY_RESERVE_FLAGS,
      MMAP_NO_FILE,
      MMAP_NO_FILE_OFFSET); 

  if(memory == MAP_FAILED)
  {
    fprintf(stderr, "Arena initialization failed: %s\n", strerror(errno));
    exit(errno);
  }

  // -
  errno = 0;
  if(mprotect(memory, CHUNK_SIZE, PROT_READ | PROT_WRITE) == -1)
  {
    fprintf(stderr, "Arena initialization failed: %s\n", strerror(errno));
    exit(errno);
  }

  // -
  arena->memory = memory;
  arena->size = CHUNK_SIZE;
  arena->position = 0;
}

function void *arena_allocate(Arena *arena, U64 size)
{

  // -
  U64 alloc_pos = arena->position;

  // -
  U64 aligned_size = 0;

  // -
  // Alignment
  if(size == MACHINE_ALIGNMENT)
  {
    aligned_size = size;
  }
  else if(size < MACHINE_ALIGNMENT)
  {
    U64 padding = MACHINE_ALIGNMENT - size;
    aligned_size = size + padding;
  }
  else if(size > MACHINE_ALIGNMENT)
  {
    if(size % MACHINE_ALIGNMENT)
    {
      U64 bl_size = (size / MACHINE_ALIGNMENT) * MACHINE_ALIGNMENT;
      aligned_size = bl_size + MACHINE_ALIGNMENT;
    }
    else
    {
      aligned_size = (size / MACHINE_ALIGNMENT) * MACHINE_ALIGNMENT;
    }
  }

  // -
  U64 new_allocation_end_pos = arena->position + aligned_size;

  // -
  if(new_allocation_end_pos > ARENA_INIT_SIZE)
  {
    fprintf(
        stderr,
        "Trying to allocate more memory, %zu bytes, than reserved in virtual memory space %u\n",
        new_allocation_end_pos,
        ARENA_INIT_SIZE);
    exit(1);
  }

  // -
  if(new_allocation_end_pos > arena->size)
  {
    U64 chunks = 1;
    if(aligned_size > CHUNK_SIZE)
    {
      chunks = (aligned_size / CHUNK_SIZE);
    }

    // Yes, we "waste" some memory here..
    U64 remainder = CHUNK_SIZE;
    U64 new_size = arena->size + (chunks * CHUNK_SIZE) + remainder;
    errno = 0;
    if(mprotect(arena->memory, new_size, PROT_READ | PROT_WRITE) == -1)
    {
      fprintf(stderr, "Arena initialization failed: %s\n", strerror(errno));
      exit(errno);
    }
    else
    {
      arena->size = new_size;
    }
  }

  // -
  arena->position = arena->position + aligned_size;

  // -
  for(U64 i = alloc_pos; i < arena->position; i++)
  {
    arena->memory[i] = 0;
  }

  // -
  void *ptr = &arena->memory[alloc_pos];


  // -
  return ptr;
}

function void arena_free(Arena *arena)
{
  for(U64 i = 0; i < arena->size; i++)
  {
    arena->memory[i] = 0;
  }

  arena->position = 0;
}

function void arena_destroy(Arena *arena)
{
  errno = 0;
  if(munmap(arena->memory, ARENA_INIT_SIZE) == -1)
  {
    fprintf(stderr, "Arena destroy failed. Exit: %s\n", strerror(errno));
    errno = 0;
    exit(1);
  }
}

// -
// String
function String *bl_string_from_cstr(Arena *arena, char *cstr)
{
  String *string = (String*)arena_allocate(arena, sizeof(String));
  string->size = strlen(cstr);
  string->str = (U8*)arena_allocate(arena, string->size);

  for(U64 i = 0; i < string->size; i++)
  {
    string->str[i] = cstr[i];
  }

  return string;
}

function String *bl_string_from_byte_array_slice(Arena *arena, ByteArray *input, U64 start, U64 end)
{
  String *result = arena_allocate(arena, sizeof(String));
  result->size = end - start;
  result->str = arena_allocate(arena, result->size);

  U64 i = 0;
  for(U64 j = start; j < end; j++)
  {
    result->str[i] = input->bytes[j];
    i++;
  }

  return result;
}

function char *bl_cstr_from_string(Arena *arena, String *string)
{

  // -
  U64 cstr_size = 0;
  for(U64 i = 0; i < string->size; i++)
  {
    if(string->str[i] == '\0')
    {
      cstr_size = i + 1;
      break;
    }
  }

  // -
  if(cstr_size == 0)
  {
    cstr_size = string->size + 1;
  }

  // -
  char *cstr = (char*)arena_allocate(arena, cstr_size);
  for(U64 i = 0; i < string->size; i++)
  {
    cstr[i] = string->str[i];
  }

  // -
  cstr[cstr_size - 1] = '\0';

  return cstr;
}

function B32 bl_string_compare(String *first, String *second)
{

  if(first->size != second->size)
  {
    return FALSE;
  }

  for(U64 i = 0; i < first->size; i++)
  {
    if(first->str[i] != second->str[i])
    {
      return FALSE;
    }
  }

  return TRUE;
}

// -
// Convert
function B32 bl_is_valid_unsigned_string_number(String *input)
{
  B32 result = FALSE;

  for(U64 i = 0; i < input->size; i++)
  {
    U8 value = input->str[i];
    if(value < '0' || value > '9')
    {
      return result;
    }
  }

  result = TRUE;
  return result;
}

function S32Result bl_s32_from_string(String *input)
{
  // -
  S32Result result = {0};

  // -
  U64 i = 0;

  // -
  B32 is_negative = FALSE;
  if(input->str[i] == '-')
  {
    is_negative = TRUE;
    i++;
  }

  if(input->str[i] == '+')
  {
    i++;
  }

  // -
  S32 number = 0;
  while(i < input->size)
  {
    if(input->str[i] >= '0' && input->str[i] <= '9')
    {
      if(number > S32_MAX / 10 || (number == S32_MAX / 10 && input->str[i] - '0' > 7))
      {
        if(is_negative)
        {
          result.value = S32_MIN;
          result.ok = TRUE;
          return result;
        }
        else
        {
          result.value = S32_MAX;
          result.ok = TRUE;
          return result;
        }

      }
      number = 10 * number + (input->str[i] - '0');
      i++;
    }
    else
    {
      result.value = 0;
      result.ok = FALSE;
      return result;
    }
  }

  if(is_negative)
  {
    result.value = -1 * number;
  }
  else
  {
    result.value = number;
  }
  result.ok = TRUE;

  return result;
}

function U64Result bl_u64_from_string(String *input)
{
  U64Result result = {0};

  if(!bl_is_valid_unsigned_string_number(input))
  {
    fprintf(stderr, "Invalid unsigned number string\n");
    return result;
  }

  U64 exp = 1;
  U64 j = input->size - 1;
  for(U64 i = 0; i < input->size; i++)
  {
    U8 pos_value = input->str[j] - 48;

    if(i == 0)
    {
      result.value = result.value + pos_value;
    }
    else
    {
      exp = exp * 10;
      U64 value = (U64)pos_value * exp;
      result.value = result.value + value;
    }

    j--;
  }

  result.ok = TRUE;
  return result;
}

function B32Result bl_b32_from_string(Arena *arena, String *input)
{
  B32Result result = {0};

  String *true_str_lowercase = bl_string_from_cstr(arena, "true");
  String *true_str_uppercase = bl_string_from_cstr(arena, "TRUE");
  String *false_str_lowercase = bl_string_from_cstr(arena, "false");
  String *false_str_uppercase = bl_string_from_cstr(arena, "FALSE");
  String *true_char_lowercase = bl_string_from_cstr(arena, "t");
  String *true_char_uppercase = bl_string_from_cstr(arena, "T");
  String *false_char_lowercase = bl_string_from_cstr(arena, "f");
  String *false_char_uppercase = bl_string_from_cstr(arena, "F");
  String *true_digit = bl_string_from_cstr(arena, "1");
  String *false_digit = bl_string_from_cstr(arena, "0");

  if(bl_string_compare(input, true_str_lowercase))
  {
    result.value = TRUE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, true_str_uppercase))
  {
    result.value = TRUE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, false_str_lowercase))
  {
    result.value = FALSE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, false_str_uppercase))
  {
    result.value = FALSE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, true_char_lowercase))
  {
    result.value = TRUE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, true_char_uppercase))
  {
    result.value = TRUE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, false_char_lowercase))
  {
    result.value = FALSE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, false_char_uppercase))
  {
    result.value = FALSE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, true_digit))
  {
    result.value = TRUE;
    result.ok = TRUE;
    return result;
  }
  else if(bl_string_compare(input, false_digit))
  {
    result.value = FALSE;
    result.ok = TRUE;
    return result;
  }

  return result;
}

function F64Result bl_f64_from_string(String *input)
{
  F64Result result = {0};

  if(input->size)
  {
    F64 number = 0.0;
    U64 i = 0;

    F64 sign = 1.0;
    if(input->str[i] == '-')
    {
      sign = -1.0;
      i++;
    }

    while(i < input->size)
    {
      U8 character = input->str[i] - (U8)'0';
      if(character < 10)
      {
        number = 10.0 * number + (F64)character;
        i++;
      }
      else
      {
        break;
      }
    }

    // -
    if(input->str[i] == '.')
    {
      i++;

      F64 c = 1.0 / 10.0;
      while(i < input->size)
      {
        U8 character = input->str[i] - (U8)'0';
        if(character < 10)
        {
          number = number + c * (F64)character;
          c *= 1.0 / 10.0;
          i++;
        }
        else
        {
          break;
        }
      }
    }

    // -
    result.value = sign * number;
    result.ok = TRUE;
  }

  return result;
}

function StringResult *bl_string_from_u64(Arena *arena, U64 value)
{
  StringResult *result = arena_allocate(arena, sizeof(StringResult));

  char buffer[MAX_U64_STRING_SIZE] = {0};

  S32 ret = snprintf(buffer, sizeof(buffer), "%lu", value);
  if(ret < 0 || (U64)ret >= sizeof(buffer))
  {
    result->ok = FALSE;
    return result;
  }

  U64 len = strlen(buffer) + 1;
  result->value = arena_allocate(arena, sizeof(String));
  result->value->size = len;
  result->value->str = arena_allocate(arena, len);
  for(U64 i = 0; i < len; i++)
  {
    result->value->str[i] = buffer[i];
  }
  result->value->str[len - 1] = '\0';

  result->ok = TRUE;
  return result;
}

#endif // DEMO_BASE_IMPLEMENTATION

#endif // DEMO_BASE_H
