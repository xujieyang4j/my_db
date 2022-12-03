#include <stdio.h>
#include <stdint.h>

int main(int argc, char const *argv[])
{
    char *buf = "insert 1 xujieyang xujieyang@163.com";
    uint32_t id;
    char username[32];
    char email[25];
    int args_assigned = sscanf(buf, "insert %d %s %s", &id, &username, &email);
    if (args_assigned < 3)
    {
        return -1;
    }
    printf("id:%d,username:%s,email:%s\n", id, username, email);
    return 0;
}
