#include <stdio.h>
#include <string.h>

int main(int argc, char const *argv[]) {
    char line[] = "insert 1 cstack foo@bar.com";
//    char *line = "insert 1 cstack foo@bar.com";
//     char *sep = " ";
    char *keyword = strtok(line, " ");
    printf("keyword:%s\n", keyword);
    char *id_string = strtok(NULL, " ");
    printf("id_string:%s\n", id_string);
    char *username = strtok(NULL, " ");
    printf("username:%s\n", username);
    char *email = strtok(NULL, " ");
    printf("email:%s\n", email);
    return 0;
}
