#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct
{
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct
{
    StatementType type;
    Row row_insert;
} Statement;

typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

// 定义获取struct属性占有的存储大小 sizeof操作符以字节形式给出了其操作数的存储大小
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

// id占有的字节数
const uint32_t ID_SIZE = size_of_attribute(Row, id);
// username占用的字节数
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
// email占用的字节数
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t PAGE_SIZE = 4096;

#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct
{
    uint32_t num_rows;
    void *pages[TABLE_MAX_PAGES];
} Table;

InputBuffer *new_input_buffer();
void print_prompt();
MetaCommandResult do_meta_command(InputBuffer *input_buffer);
void read_input(InputBuffer *input_buffer);
void close_input_buffer(InputBuffer *input_buffer);
PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement);
ExecuteResult execute_statement(Statement *statement, Table *table);
ExecuteResult execute_insert(Statement *statement, Table *table);
ExecuteResult execute_select(Statement *statement, Table *table);
void serialize_row(Row *source, void *destination);
void deserialize_row(void *source, Row *destination);
Table *new_table();
void *row_slot(Table *table, uint32_t row_num);
void print_row(Row *row);

InputBuffer *new_input_buffer()
{
    InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}

void print_prompt()
{
    printf("db > ");
}

MetaCommandResult do_meta_command(InputBuffer *input_buffer)
{
    if (strcmp(input_buffer->buffer, ".exit") == 0)
    {
        close_input_buffer(input_buffer);
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement)
{
    // 比如：insert 1 cstack foo@bar.com
    if (strncmp(input_buffer->buffer, "insert", 6) == 0)
    {
        statement->type = STATEMENT_INSERT;
        // 记录一个错误，user_name和email是char数组，可认为是指针，不用加&
        // int args_assigned = sscanf(input_buffer->buffer, "insert %d %s %s", &(statement->row_insert.id), &(statement->row_insert.username), &(statement->row_insert.email));
        // TODO 去掉前后空格
        int args_assigned = sscanf(input_buffer->buffer, "insert %d %s %s", &(statement->row_insert.id), statement->row_insert.username, statement->row_insert.email);

        if (args_assigned < 3)
        {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "select", 6) == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_statement(Statement *statement, Table *table)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        return execute_insert(statement, table);
        break;
    case (STATEMENT_SELECT):
        return execute_select(statement, table);
        break;
    }
}

ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if (table->num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }
    Row *row_to_insert = &(statement->row_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
}

void print_row(Row *row)
{
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

void serialize_row(Row *source, void *destination)
{
    // void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
    // The memcpy() function shall copy n bytes from the object pointed to by s2 into the object pointed to by s1.
    // If copying takes place between objects that overlap, the behavior is undefined.
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserialize_row(void *source, Row *destination)
{
    // void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
    // The memcpy() function shall copy n bytes from the object pointed to by s2 into the object pointed to by s1.
    // If copying takes place between objects that overlap, the behavior is undefined.
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

// row_slot:返回当前page指针指向的内存地址（或者说指向第几row），用内存偏移量表示
void *row_slot(Table *table, uint32_t row_num)
{
    // 1. 定位当前插入的行，在哪个page中
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void *page = table->pages[page_num];
    // page == NULL说明当前page未插入过row，执行初始化
    if (page == NULL)
    {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

void read_input(InputBuffer *input_buffer)
{
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
    printf("bytes_read:%zu\n", bytes_read);
    if (bytes_read <= 0)
    {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    // TODO add by 徐洁阳 Ignore trailing newline ???
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
    printf("input_buffer#buffer:%s, input_buffer#buffer_length:%zu, input_buffer#input_length:%zu\n", input_buffer->buffer, input_buffer->buffer_length, input_buffer->input_length);
}

void close_input_buffer(InputBuffer *input_buffer)
{
    free(input_buffer->buffer);
    free(input_buffer);
}

Table *new_table()
{
    Table *table = (Table *)malloc(sizeof(Table));
    table->num_rows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table *table)
{
    for (int i = 0; table->pages[i] != NULL; i++)
    {
        free(table->pages[i]);
    }
    free(table);
}

int main(int argc, char const *argv[])
{
    // // 调试代码
    // Table *table = new_table();
    // InputBuffer *input_buffer = new_input_buffer();
    // input_buffer->buffer = "insert 1 cstack foo@bar.com";
    // // input_buffer->input_length = strlen(input_buffer->buffer) - 1;
    // // input_buffer->buffer[input_buffer->buffer_length] = 0;
    // Statement statement;
    // PrepareResult pr = prepare_statement(input_buffer, &statement);
    // ExecuteResult er = execute_statement(&statement, table);

    


    初始化Table
    Table *table = new_table();

    InputBuffer *input_buffer = new_input_buffer();
    while (true)
    {
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer))
            {
            case META_COMMAND_SUCCESS:
                continue;
            case META_COMMAND_UNRECOGNIZED_COMMAND:
                printf("Unrecognized command '%s'.\n", input_buffer->buffer);
                continue;
            }
        }
        Statement statement;
        switch (prepare_statement(input_buffer, &statement))
        {
        case PREPARE_SUCCESS:
            break;
        case PREPARE_SYNTAX_ERROR:
            printf("Syntax error. Could not parse statement.\n");
            continue;
        case PREPARE_UNRECOGNIZED_STATEMENT:
            printf("Unrecognized statmenr '%s'.\n", input_buffer->buffer);
            continue;
        }
        switch (execute_statement(&statement, table))
        {
        case (EXECUTE_SUCCESS):
            printf("Executed.\n");
            break;
        case (EXECUTE_TABLE_FULL):
            printf("Error: Table full.\n");
            break;
        }
    }
    return 0;
}
