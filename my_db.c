#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// 定义获取struct属性的size
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

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
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct
{
    StatementType type;
} Statement;

InputBuffer *new_input_buffer();
void print_prompt();
MetaCommandResult do_meta_command(InputBuffer *input_buffer);
void read_input(InputBuffer *input_buffer);
void close_input_buffer(InputBuffer *input_buffer);
PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement);
void execute_statement(Statement *statement);

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
    if (strncmp(input_buffer->buffer, "insert", 6) == 0)
    {
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "select", 6) == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement *statement)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        printf("Execute insert.\n");
        break;
    case (STATEMENT_SELECT):
        printf("Execute select.\n");
        break;
    }
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
    // Ignore trailing newline ???
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
    printf("input_buffer#buffer:%s, input_buffer#buffer_length:%zu, input_buffer#input_length:%zu\n", input_buffer->buffer, input_buffer->buffer_length, input_buffer->input_length);
}

void close_input_buffer(InputBuffer *input_buffer)
{
    free(input_buffer->buffer);
    free(input_buffer);
}

int main(int argc, char const *argv[])
{
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
        case PREPARE_UNRECOGNIZED_STATEMENT:
            printf("Unrecognized statmenr '%s'.\n", input_buffer->buffer);
            continue;
        }
        execute_statement(&statement);
        // printf("Executed.\n");
    }
    return 0;
}
