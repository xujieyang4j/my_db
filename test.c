#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    uint32_t num_rows;
    void *pages[100];
} Table2;

// 定义获取struct属性占有的存储大小 sizeof操作符以字节形式给出了其操作数的存储大小
#define size_of_attribute2(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

int main(int argc, char const *argv[])
{
    Table2 *table = (Table2 *)malloc(sizeof(Table2));
    table->num_rows = 0;
    for (uint32_t i = 0; i < 100; i++)
    {
        table->pages[i] = NULL;
    }

    uint32_t ROW_SIZE = 291;
    uint32_t PAGE_SIZE = 4096;
    uint32_t row_num = 0;
    uint32_t ROWS_PER_PAGE = PAGE_SIZE / 291;

    for (int row_num = 0; row_num < 21; row_num++)
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
        printf("row_num:%d, row_offset:%d, byte_offset:%d\n", row_num, row_offset, byte_offset);
    }
    return 0;
}
