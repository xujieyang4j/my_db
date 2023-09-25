#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "my_db.h"


InputBuffer *new_input_buffer() {
    InputBuffer *input_buffer = (InputBuffer *) malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}

void print_prompt() {
    printf("db > ");
}

void print_constants() {
    printf("ROW_SIZE: %d\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void print_leaf_node(void *node) {
    uint32_t num_cells = *leaf_node_num_cells(node);
    printf("leaf (size %d)\n", num_cells);
    for (uint32_t i = 0; i < num_cells; i++) {
        uint32_t key = *leaf_node_key(node, i);
        printf("  - %d : %d\n", i, key);
    }
}

MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_input_buffer(input_buffer);
        db_close(table);
        exit(EXIT_SUCCESS);
    } else if (strcmp(input_buffer->buffer, ".btree") == 0) {
        printf("Tree:\n");
        print_leaf_node(get_page(table->pager, 0));
        return META_COMMAND_SUCCESS;
    } else if (strcmp(input_buffer->buffer, ".constants") == 0) {
        printf("Constants:\n");
        print_constants();
        return META_COMMAND_SUCCESS;
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

Table *db_open(const char *file_name) {
    Pager *pager = pager_open(file_name);
//    uint32_t num_rows = pager->file_length / ROW_SIZE;

    Table *table = malloc(sizeof(Table));
    table->pager = pager;
//    table->num_rows = num_rows;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        // New database file. Initialize page 0 as leaf node.
        void *root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
    }

//    Table *table = (Table *) malloc(sizeof(Table));
//    table->num_rows = 0;
//    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
//        table->pages[i] = NULL;
//    }
    return table;
}

Pager *pager_open(const char *file_name) {
    int fd = open(file_name,
                  O_RDWR |  // Read/Write mode
                  O_CREAT,  // Create file if it does not exist
                  S_IWUSR | // User write permission
                  S_IRUSR); // User read permission
    if (fd == -1) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }
    off_t file_length = lseek(fd, 0, SEEK_END);

    Pager *pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = (file_length / PAGE_SIZE);

    for (u_int32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

Cursor *table_start(Table *table) {
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
//    cursor->row_num = 0;
//    cursor->end_of_table = (table->num_rows == 0);
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

Cursor *table_end(Table *table) {
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
//    cursor->row_num = table->num_rows;
    cursor->page_num = table->root_page_num;

    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->cell_num = num_cells;
    cursor->end_of_table = true;

    return cursor;
}

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement) {
    // 比如：insert 1 cstack foo@bar.com
    if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        return prepare_insert(input_buffer, statement);
    }
    if (strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement) {
    statement->type = STATEMENT_INSERT;
    /*
    // 记录一个错误，user_name和email是char数组，可认为是指针，不用加&
    // int args_assigned = sscanf(input_buffer->buffer, "insert %d %s %s", &(statement->row_insert.id), &(statement->row_insert.username), &(statement->row_insert.email));
    // TODO 去掉前后空格
    int args_assigned = sscanf(input_buffer->buffer, "insert %d %s %s", &(statement->row_insert.id), statement->row_insert.username, statement->row_insert.email);

    if (args_assigned < 3)
    {
        return PREPARE_SYNTAX_ERROR;
    }
    return PREPARE_SUCCESS;
    */
//    char *keyword = strtok(input_buffer->buffer, " ");
    strtok(input_buffer->buffer, " ");
    char *id_string = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    char *email = strtok(NULL, " ");
    if (id_string == NULL || username == NULL || email == NULL) {
        return PREPARE_SYNTAX_ERROR;
    }
    int id = atoi(id_string);
//    int id = strtol(id_string, NULL, 10);
    if (id < 1) {
        return PREPARE_NEGATIVE_ID;
    }
//    printf("strlen(username):%lu\n", strlen(username));
    if (strlen(username) > COLUMN_USERNAME_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > COLUMN_EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }
    statement->row_insert->id = id;
    // 报错，改成strcpy
//     statement->row_insert.username = username;
    // statement->row_insert.email = email;
    strcpy(statement->row_insert->username, username);
    strcpy(statement->row_insert->email, email);

//    printf("id:%d,username:%s,email:%s\n", statement->row_insert.id, statement->row_insert.username, statement->row_insert.email);
    return PREPARE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            return execute_select(statement, table);
    }
}

ExecuteResult execute_insert(Statement *statement, Table *table) {
//    if (table->num_rows >= TABLE_MAX_ROWS) {
//        return EXECUTE_TABLE_FULL;
//    }
    void *node = get_page(table->pager, table->root_page_num);
    if ((*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS)) {
        return EXECUTE_TABLE_FULL;
    }

    Row *row_to_insert = statement->row_insert;
    Cursor *cursor = table_end(table);

//    serialize_row(row_to_insert, row_slot(table, table->num_rows));
//    serialize_row(row_to_insert, cursor_value(cursor));
//    table->num_rows += 1;

    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

    free(cursor);
    return EXECUTE_SUCCESS;
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
    void *node = get_page(cursor->table->pager, cursor->page_num);

    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        printf("Need to implement splitting a leaf node.\n");
        exit(EXIT_FAILURE);
    }

    if (cursor->cell_num < num_cells) {
        // Make room for new cell
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

ExecuteResult execute_select(Statement *statement, Table *table) {
    Cursor *cursor = table_start(table);

    Row row;
    while (!cursor->end_of_table) {
        deserialize_row(cursor_value(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }

//    for (uint32_t i = 0; i < table->num_rows; i++) {
//        deserialize_row(row_slot(table, i), &row);
//        print_row(&row);
//    }
    return EXECUTE_SUCCESS;
}

void serialize_row(Row *source, void *destination) {
    // void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
    // The memcpy() function shall copy n bytes from the object pointed to by s2 into the object pointed to by s1.
    // If copying takes place between objects that overlap, the behavior is undefined.
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserialize_row(void *source, Row *destination) {
    // void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
    // The memcpy() function shall copy n bytes from the object pointed to by s2 into the object pointed to by s1.
    // If copying takes place between objects that overlap, the behavior is undefined.
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

// row_slot:返回当前page指针指向的内存地址（或者说指向第几row），用内存偏移量表示
//void *row_slot(Table *table, uint32_t row_num) {
void *cursor_value(Cursor *cursor) {
//    uint32_t row_num = cursor->row_num;
//
//    // 1. 定位当前插入的行，在哪个page中
//    uint32_t page_num = row_num / ROWS_PER_PAGE;
////    void* page = table->pages[page_num];
////    // page == NULL说明当前page未插入过row，执行初始化
////    if (page == NULL) {
////        page = table->pages[page_num] = malloc(PAGE_SIZE);
////    }
////    void *page = get_page(table->pager, page_num);
//    void *page = get_page(cursor->table->pager, page_num);
//    uint32_t row_offset = row_num % ROWS_PER_PAGE;
//    uint32_t byte_offset = row_offset * ROW_SIZE;
//
//    return page + byte_offset;

    uint32_t page_num = cursor->page_num;
    void *page = get_page(cursor->table->pager, page_num);
    return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor *cursor) {
//    cursor->row_num += 1;
//    if (cursor->row_num == cursor->table->num_rows) {
//        cursor->end_of_table = true;
//    }

    uint32_t page_num = cursor->page_num;
    void *node = get_page(cursor->table->pager, page_num);
    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        cursor->end_of_table = true;
    }
}

void *get_page(Pager *pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        // Cache miss. Allocate memory and load from file.
        void *page = malloc(PAGE_SIZE);
        u_int32_t num_pages = pager->file_length / PAGE_SIZE;

        // We might save a partial page at the end of the file
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num <= num_pages) {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }

    return pager->pages[page_num];
}

void print_row(Row *row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

void read_input(InputBuffer *input_buffer) {
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
//    printf("read_input:bytes_read:%zu\n", bytes_read);
//    printf("read_input:input_buffer#buffer:%s\n", input_buffer->buffer);
    if (bytes_read <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    // Ignore trailing newline
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = '\0';
//    printf("read_input:input_buffer#buffer:%s, input_buffer#buffer_length:%zu, input_buffer#input_length:%zu\n",
//           input_buffer->buffer, input_buffer->buffer_length, input_buffer->input_length);
}

void close_input_buffer(InputBuffer *input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

void *db_close(Table *table) {
    Pager *pager = table->pager;
//    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

//    for (uint32_t i = 0; i < num_full_pages; i++) {
    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }
//        pager_flush(pager, i, PAGE_SIZE);
        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // There may be a partial page to write to the end of the file
    // This should not be needed after we switch to a B-tree
//    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
//    if (num_additional_rows > 0) {
//        uint32_t page_num = num_full_pages;
//        pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
//        free(pager->pages[page_num]);
//        pager->pages[page_num] = NULL;
//    }

    int result = close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void *page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }

    free(pager);
    free(table);
}

//void pager_flush(Pager *pager, uint32_t page_num, uint32_t size) {
void pager_flush(Pager *pager, uint32_t page_num) {
    if (pager->pages[page_num] == NULL) {
        printf("Tried to flush null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

//    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);
    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
    if (bytes_written == -1) {
        printf("Error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

uint32_t *leaf_node_num_cells(void *node) {
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void *leaf_node_cell(void *node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

uint32_t *leaf_node_value(void *node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void *node) {
    *leaf_node_num_cells(node) = 0;
}

//void free_table(Table *table) {
//    for (int i = 0; table->pages[i] != NULL; i++) {
//        free(table->pages[i]);
//    }
//    free(table);
//}

int main(int argc, char const *argv[]) {
    // 初始化Table
//    Table *table = new_table();
    if (argc < 2) {
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }

    const char *file_name = argv[1];
    Table *table = db_open(file_name);

    InputBuffer *input_buffer = new_input_buffer();
    while (true) {
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer[0] == '.') {
            switch (do_meta_command(input_buffer, table)) {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    printf("Unrecognized command '%s'.\n", input_buffer->buffer);
                    continue;
            }
        }
        Statement statement;
        switch (prepare_statement(input_buffer, &statement)) {
            case PREPARE_SUCCESS:
                break;
            case PREPARE_SYNTAX_ERROR:
                printf("Syntax error. Could not parse statement.\n");
                continue;
            case (PREPARE_STRING_TOO_LONG):
                printf("String is too long.\n");
                continue;
            case (PREPARE_NEGATIVE_ID):
                printf("ID must be positive.\n");
                continue;
            case PREPARE_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized statement '%s'.\n", input_buffer->buffer);
                continue;
        }
        switch (execute_statement(&statement, table)) {
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
