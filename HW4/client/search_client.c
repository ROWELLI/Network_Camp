#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>

#define BUF_SIZE 1
#define COLOR_RED "\033[38;2;255;0;0m"
#define COLOR_RESET "\033[0m"

void error_handling(char *message);

typedef struct {
    char input[100]; 
    char search_result[100]; 
    int result_num;
} Top10;

int main(int argc, char *argv[]) {
    int sd;
    struct sockaddr_in serv_adr;
    char let;

    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error");

    struct termios tattr;

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Not a terminal.\n");
        exit(EXIT_FAILURE);
    }

    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON | ECHO);
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tattr);

    while (1) {
        let = getchar();
        write(sd, &let, 1);

        if (let == '\n') {
            break;
        }
        Top10 top10[10]; 
        int read_cnt = read(sd, top10, sizeof(top10));
        printf("%d", top10[0].result_num);
        if (read_cnt > 0) {
            printf("\033[2J\033[H");
            printf("Input: %s%s%s\n", COLOR_RED, top10[0].input, COLOR_RESET);
            printf("---------------------\n");

            for (int i = 0; i < read_cnt / sizeof(Top10); i++) {
                if (strcmp(top10[i].search_result, "Type input") == 0) {
                    printf("%s\n", top10[i].search_result);
                    continue;
                }
                int input_len = strlen(top10[0].input);
                int result_len = strlen(top10[i].search_result);
                for (int j = 0; j < result_len; j++) {
                    if (j <= result_len - input_len && strncmp(&top10[i].search_result[j], top10[0].input, input_len) == 0) {
                        printf("%s%.*s%s", COLOR_RED, input_len, &top10[i].search_result[j], COLOR_RESET);
                        j += input_len - 1;
                    } else {
                        putchar(top10[i].search_result[j]);
                    }
                }
                printf("\n");
            }
        }
    }

    close(sd);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
