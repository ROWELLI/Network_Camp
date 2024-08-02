#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_LINES 1000

void error_handling(char *message);
void *read_write_letter(void *arg);
void sort_letter(const char* letter, int clnt_sd, const char* filename);

const char *filename;

typedef struct {
    char text[BUF_SIZE];
    int number;
} Entry;

typedef struct {
    char input[BUF_SIZE];
    char search_result[BUF_SIZE];
    int result_num;
} Top10;

int compare(const void *a, const void *b) {
    Entry *entryA = (Entry *)a;
    Entry *entryB = (Entry *)b;
    return entryB->number - entryA->number;
}

int main(int argc, char *argv[]) {
    int serv_sd, clnt_sd;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;

    if (argc != 3) {
        printf("Usage: %s <port> <filename>\n", argv[0]);
        exit(1);
    }

    filename = argv[2];

    serv_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sd == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sd, 5) == -1)
        error_handling("listen() error");

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if (clnt_sd == -1)
            error_handling("accept() error");

        pthread_create(&t_id, NULL, read_write_letter, (void*)&clnt_sd);
        pthread_detach(t_id);
    }

    close(serv_sd);
    return 0;
}

void *read_write_letter(void *arg) {
    int clnt_sd = *((int*)arg);
    int read_cnt;
    char let;
    char letter[BUF_SIZE] = {0}; // 버퍼 초기화
    int len = 0;

    while (1) {
        read_cnt = read(clnt_sd, &let, 1);
        if (read_cnt <= 0 || let == '\n') {
            break;
        }

        if (let == 127) {
            if (len > 0) {
                letter[--len] = '\0'; 
            }
            if (len == 0) {
                Top10 type_input;
                strcpy(type_input.input, letter);
                strcpy(type_input.search_result, "Type input");
                type_input.result_num = 0;
                write(clnt_sd, &type_input, sizeof(Top10));
                continue;
            }
        } else {
            if (len < BUF_SIZE - 1) {
                letter[len++] = let;
                letter[len] = '\0';
            }
        }
        printf("Message from client: %s\n", letter);
        sort_letter(letter, clnt_sd, filename);
    }

    close(clnt_sd);
    return NULL;
}

void sort_letter(const char* letter, int clnt_sd, const char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        error_handling("Failed to open file");
    }

    Entry entries[MAX_LINES];
    int count = 0;
    char buffer[BUF_SIZE];

    // 파일을 읽어서 구조체 배열에 저장
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *ptr = strrchr(buffer, ' ');
        if (ptr != NULL) {
            *ptr = '\0'; 
            entries[count].number = atoi(ptr + 1);
            strcpy(entries[count].text, buffer);
            count++;
        }
    }
    fclose(file);

    // 입력된 문자를 포함하는 항목 필터링
    Entry match_result[MAX_LINES];
    int filtered_count = 0;
    for (int i = 0; i < count; i++) {
        if (strstr(entries[i].text, letter) != NULL) {
            match_result[filtered_count++] = entries[i];
        }
    }

    // 입력이 다 지워졌을 경우
    if (filtered_count == 0) {
        Top10 no_type;
        strcpy(no_type.input, letter);
        strcpy(no_type.search_result, "Nothing found");
        no_type.result_num = 0;
        write(clnt_sd, &no_type, sizeof(Top10));
        return;
    }

    // 내림차순 정렬
    qsort(match_result, filtered_count, sizeof(Entry), compare);

    // 정렬된 배열 전송
    Top10 top10[10];

    int top_count = (filtered_count < 10) ? filtered_count : 10;
    printf("Filtered and sorted entries:\n");
    for (int i = 0; i < top_count; i++) {
        strcpy(top10[i].input, letter);
        strcpy(top10[i].search_result, match_result[i].text);
        top10[i].result_num = top_count;
        printf("%s %d\n", match_result[i].text, match_result[i].number);
    }
    write(clnt_sd, top10, sizeof(Top10) * top_count);
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
