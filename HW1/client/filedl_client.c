#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 2048

void error_handling(char *message);

struct File {
    char fname[256];
    int fsize;
};

int main(int argc, char* argv[]) {
    int sock, file_num;
    struct sockaddr_in serv_addr;
    struct File f[20];
    int file_index;
    int read_file;
    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error!");

    printf("---------------------------------------------------------\n");
    // number of files
    if (read(sock, &file_num, sizeof(int)) == -1) {
        fprintf(stderr, "file number read failed");
        exit(-1);
    }
    // printf("number of files: %d\n", file_num);

    // get file struct
    int get_struct = 0;
    while (get_struct < sizeof(struct File) * file_num) {
        read_file = read(sock, ((char*)f) + get_struct, sizeof(struct File) * file_num - get_struct);
        if (read_file <= 0) {
            fprintf(stderr, "file info read failed");
            exit(-1);
        }
        get_struct += read_file;
    }

    for (int i = 0; i < file_num; i++) {
        printf("%d. file name: %s, file size: %d bytes\n", i + 1, f[i].fname, f[i].fsize);
    }
    printf("---------------------------------------------------------\n");
    printf("\n");

    while (1) {
        printf("Choose a number of file to get (Quit: 0): ");
        scanf("%d", &file_index);

        if (write(sock, &file_index, sizeof(int)) == -1) {
            fprintf(stderr, "file index write failed");
            exit(-1);
        }

        if (file_index == 0) {
            printf("Bye!\n");
            break;
        } 

        fp = fopen(f[file_index - 1].fname, "wb");

        // get file content
        int total_received = 0;
        while (total_received < f[file_index-1].fsize) { 
            read_cnt = read(sock, buf, BUF_SIZE);
            if (read_cnt == -1) {
                fprintf(stderr, "file read failed\n");
                exit(-1);
            }
            fwrite(buf, 1, read_cnt, fp);
            total_received += read_cnt;
        }

        fclose(fp);
    }

    close(sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
