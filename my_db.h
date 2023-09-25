//
// Created by bytedance on 2023/9/25.
//

#include <stdio.h>

#ifndef MY_DB_MY_DB_H
#define MY_DB_MY_DB_H

#endif //MY_DB_MY_DB_H

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct {
    uint32_t id;
    // 注意+1
    //  C strings are supposed to end with a null character, which we should allocate space for.
    // char username[COLUMN_USERNAME_SIZE];
    // char email[COLUMN_EMAIL_SIZE];
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

typedef struct {
    StatementType type;
    Row *row_insert;
} Statement;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

// 定义获取struct属性占有的存储大小 sizeof操作符以字节形式给出了其操作数的存储大小
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

// ID占用的字节数
const uint32_t ID_SIZE = size_of_attribute(Row, id);
// username占用的字节数
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
// email占用的字节数
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);

// ID的偏移量
const uint32_t ID_OFFSET = 0;
// username的偏移量=ID的偏移量+id占有的字节数
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
// email的偏移量=username的偏移量+username占用的字节数
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
// 每行占用的字节数
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
//  每页的字节数
const uint32_t PAGE_SIZE = 4096;

#define TABLE_MAX_PAGES 100
// 每页存储的行数
//const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
// table最大行
//const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    // 页数
    uint32_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
//    uint32_t num_rows;
//    void *pages[TABLE_MAX_PAGES];
    Pager *pager;
    uint32_t root_page_num;
} Table;

typedef struct {
    Table *table;
//    uint32_t row_num;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
} Cursor;

typedef enum {
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

/**
 * Common Node Header Layout
 */
const uint32_t NODE_TYPE_SIZE = sizeof(u_int8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(u_int8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(u_int32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/**
 * Leaf Node Header Layout
 */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/**
 * Leaf Node Body Layout
 */
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

InputBuffer *new_input_buffer();

void print_prompt();

MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table);

void read_input(InputBuffer *input_buffer);

void close_input_buffer(InputBuffer *input_buffer);

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement);

PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement);

ExecuteResult execute_statement(Statement *statement, Table *table);

ExecuteResult execute_insert(Statement *statement, Table *table);

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value);

ExecuteResult execute_select(Statement *statement, Table *table);

void serialize_row(Row *source, void *destination);

void deserialize_row(void *source, Row *destination);

Table *db_open(const char *file_name);

Pager *pager_open(const char *file_name);

Cursor *table_start(Table *table);

Cursor *table_end(Table *table);

void *cursor_value(Cursor *cursor);

void cursor_advance(Cursor *cursor);

void *get_page(Pager *pager, uint32_t page_num);

void *db_close(Table *table);

//void pager_flush(Pager *pager, uint32_t page_num, uint32_t size);
void pager_flush(Pager *pager, uint32_t page_num);

uint32_t *leaf_node_num_cells(void *node);

void *leaf_node_cell(void *node, uint32_t cell_num);

uint32_t *leaf_node_key(void *node, uint32_t cell_num);

uint32_t *leaf_node_value(void *node, uint32_t cell_num);

void initialize_leaf_node(void *node);

//void *row_slot(Table *table, uint32_t row_num);

void print_row(Row *row);