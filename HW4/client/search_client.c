#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>

#define BUF_SIZE 1
#define BACKSPACE 127 // 터미널에서 백스페이스 ASCII 값

void error_handling(char *message);

typedef struct {
    char input[100]; // BUF_SIZE를 100으로 증가
    char search_result[100]; // BUF_SIZE를 100으로 증가
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
    tattr.c_lflag &= ~(ICANON | ECHO); // Canonical 모드 비활성화, 입력 문자 출력 비활성화
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tattr);

    while (1) {
        let = getchar();
        write(sd, &let, 1);

        if (let == '\n') {
            break;
        }

        Top10 top10[10]; // 최대 10개의 결과를 받을 수 있도록 설정
        int read_cnt = read(sd, top10, sizeof(top10));
        if (read_cnt > 0) {
            for (int i = 0; i < read_cnt / sizeof(Top10); i++) {
                if (strcmp(top10[i].search_result, "Type input") == 0) {
                    printf("Type input\n");
                } else if (strcmp(top10[i].search_result, "Nothing found") == 0) {
                    printf("Input: %s, Nothing found\n", top10[i].input);
                } else {
                    printf("Input: %s, Result: %s %d\n", top10[i].input, top10[i].search_result, top10[i].result_num);
                }
            }
        }
        printf("서버 응답: %c\n", let);
    }

    close(sd);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
