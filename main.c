#include <sys/stat.h>

#define DEMO_BASE_IMPLEMENTATION
#include "demo_base.h"

#define COLUMNS 5
enum ValueType
{
  U64_TYPE = 1,
  S32_TYPE = 2,
  STRING_TYPE = 3,
  B32_TYPE = 4,
  F64_TYPE = 5,
};

typedef struct Row Row;
struct Row
{
  U64 unsigned_integer_64;
  S32 signed_integer_32;
  String *literal;
  B32 bool_32;
  F64 float_64;
};

typedef struct Rows Rows;
struct Rows
{
  Row **items;
  U64 count;
};

typedef struct Column Column;
struct Column
{
  enum ValueType type; 
  U32 start_pos;
  U32 end_pos;
  U32 row;
  Column *next;
};

typedef struct ColumnList ColumnList;
struct ColumnList
{
  Column *start;
  U32 column_count;
};

function ByteArray *read_entire_file(Arena *arena, char *filename)
{
  ByteArray *result = arena_allocate(arena, sizeof(ByteArray));

  FILE *file = fopen(filename, "r");
  if(file)
  {
    struct stat file_stats;
    stat(filename, &file_stats);

    result->size = file_stats.st_size;
    result->bytes = arena_allocate(arena, result->size);

    U64 elements_to_read = 1;
    U64 elements_read = fread(result->bytes, result->size, elements_to_read, file);

    if(elements_read != elements_to_read)
    {
      if(ferror(file))
      {
        fprintf(stderr, "Error: Failed to read %s\n", filename);
      }

      result->size = 0;
    }

    fclose(file);
  }
  else
  {
    fprintf(stderr, "Error: Failed to read %s\n", filename);
  }

  return result;
}

function B32 in_range(U64 pos, U64 max)
{
  B32 result = TRUE;

  if(pos >= max)
  {
    result = FALSE;
  }

  return result;
}

function B32 is_new_line(U8 character)
{
  B32 result = FALSE;
  if(character == '\n')
  {
    result = TRUE;
  }

  return result;
}

function ColumnList *extract_columns(Arena *arena, ByteArray *input)
{
  ColumnList *result = arena_allocate(arena, sizeof(ColumnList));

  Column *previous_column = {0};

  U32 column_start = 0;
  U32 column_end = 0;
  U8 column_index = 0;

  U32 row_count = 0;
  U32 column_count = 0;

  while(in_range(column_end, input->size))
  {
    if(input->bytes[column_end] == ',' || is_new_line(input->bytes[column_end]))
    {

      // -
      if(is_new_line(input->bytes[column_end]))
      {
        row_count++;
      }

      // -
      column_count++;

      // -
      if(column_index == COLUMNS)
      {
        column_index = 1;
      }
      else
      {
        column_index++;
      }

      // -
      Column *column = arena_allocate(arena, sizeof(Column));
      column->start_pos = column_start;
      column->end_pos = column_end;

      // -
      switch(column_index)
      {
        case U64_TYPE: { column->type = U64_TYPE; } break;
        case S32_TYPE: { column->type = S32_TYPE; }  break;
        case STRING_TYPE: { column->type = STRING_TYPE; } break;
        case B32_TYPE: { column->type = B32_TYPE; } break;
        case F64_TYPE: { column->type = F64_TYPE; } break;
        default: { fprintf(stderr, "Unkown type for column index %u\n", column_index); }
      }

      // -
      if(row_count == 0 && column_index == 1)
      {
        result->start = column;
        previous_column = column;
      }
      else
      {
        previous_column->next = column;
        previous_column = column;
      }

      // -
      column_start = column_end + 1;
      column_end = column_start;
    }
    else
    {
      column_end++;
    }
  }

  // -
  result->column_count = column_count;
  return result;
}

function Row *parse_row(Arena *arena, ByteArray *csv_data, Column *item)
{
  Row *result = arena_allocate(arena, sizeof(Row));

  B32 keep_parsing = TRUE;
  U32 count = 0;
  while(item && keep_parsing && count < COLUMNS)
  {
    // -
    String *value_str = bl_string_from_byte_array_slice(arena, csv_data, item->start_pos, item->end_pos);

    // -
    switch(item->type)
    {
      case U64_TYPE:
        {
          U64Result u64_result = bl_u64_from_string(value_str);
          if(u64_result.ok)
          {
            result->unsigned_integer_64 = u64_result.value;
          }
          else
          {
            fprintf(stderr, "Parsing failed!\n");
            keep_parsing = FALSE;
          }
        }
        break;

      case S32_TYPE:
        {
          S32Result s32_result = bl_s32_from_string(value_str);
          if(s32_result.ok)
          {
            result->signed_integer_32 = s32_result.value;
          }
          else
          {
            fprintf(stderr, "Parsing failed!\n");
            keep_parsing = FALSE;
          }
        }
        break;

      case STRING_TYPE:
        {
          result->literal = value_str;
        }
        break;

      case B32_TYPE:
        {
          B32Result b32_result = bl_b32_from_string(arena, value_str);
          if(b32_result.ok)
          {
            result->bool_32 = b32_result.value;
          }
          else
          {
            fprintf(stderr, "Parsing failed!\n");
            keep_parsing = FALSE;
          }
        }
        break;

      case F64_TYPE:
        {
          F64Result f64_result = bl_f64_from_string(value_str);
          if(f64_result.ok)
          {
            result->float_64 = f64_result.value;
          }
          else
          {
            fprintf(stderr, "Parsing failed!\n");
            keep_parsing = FALSE;
          }
        }
        break;
    }

    // -
    count++;
    item = item->next;
  }

  return result;
}

function Rows *parse_data(Arena *arena, ByteArray *csv_data, ColumnList *column_list)
{

  // -
  U32 row_count = column_list->column_count / COLUMNS;
  Rows *rows = arena_allocate(arena, sizeof(Rows));
  rows->count = row_count;
  rows->items = arena_allocate(arena, rows->count * sizeof(Rows*));

  // -
  Column *item = column_list->start;
  for(U32 i = 0; i < row_count; i++)
  {
    // -
    Row *row = parse_row(arena, csv_data, item);
    rows->items[i] = row;

    // -
    // Fast forword to next set of columns to parse a row
    for(U32 j = 0; j < COLUMNS; j++)
    {
      item = item->next;
    }
  }

  // -
  return rows;
}

function void print_rows(Arena *arena, Rows *rows)
{
  for(U64 i = 0; i < rows->count; i++)
  {
    Row *row = rows->items[i];
    printf(
        "%lu,%d,%s,%s,%f\n",
        row->unsigned_integer_64,
        row->signed_integer_32,
        bl_cstr_from_string(arena, row->literal),
        row->bool_32 ? "true" : "false",
        row->float_64);
  }
}

S32 main(S32 arg_count, char **args_list)
{
  // -
  S32 result = 1;

  // -
  Arena arena = {0};
  arena_init(&arena);

  // -
  if(arg_count == 2)
  {
    char *filename = args_list[1];
    ByteArray *csv_data = read_entire_file(&arena, filename);
    if(csv_data->size)
    {
      ColumnList *column_list = extract_columns(&arena, csv_data);
      Rows *rows = parse_data(&arena, csv_data, column_list);
      print_rows(&arena, rows);
    }

    result = 0; 
  }
  else
  {
    fprintf(stderr, "Usage: ./demo filename.csv\n");
  }

  // -
  // NOTE(David): Not strictly necessary but for demo purposes.
  // OS will free all memory anyway on exit.
  arena_free(&arena);

  // -
  return result;
}
