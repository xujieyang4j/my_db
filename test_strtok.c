#include <stdio.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    char line[] = "LINE TO BE SEPARATED";
    char *sep = " ";
    char *keyword = strtok(line, sep);
    printf("keyword:%s\n", keyword);
    char *id_string = strtok(NULL, sep);
    printf("id_string:%s\n", id_string);
    char *username = strtok(NULL, sep);
    printf("username:%s\n", username);
    char *email = strtok(NULL, sep);
    printf("email:%s\n", email);
    return 0;
}
