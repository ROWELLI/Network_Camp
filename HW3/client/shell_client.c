#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char *message);
void cmd_ls(int sock, char *message);
void cmd_ul(int sock, char *file_name);

struct File {
    char fname[256];
    int fsize;
};

int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr;

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    
    sock = socket(PF_INET, SOCK_STREAM, 0);   
    if (sock == -1)
        error_handling("socket() error");
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));
    
    if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error!");
    else
        puts("Connected...........");
    
    while (1) 
    {
        fputs("Input message(ls, cd <dirname>, ul, dl, q(quit)): ", stdout);
        fgets(message, BUF_SIZE, stdin);
        int str_len = write(sock, message, strlen(message));
        
        if (!strcmp(message,"q\n") || !strcmp(message,"Q\n"))
            break;

        if (strncmp(message, "ls", 2) == 0){
            cmd_ls(sock, message);
        }

        if (strncmp(message, "dl", 2) == 0){
            int file_num, read_file;
            struct File f[20]; // File structure

            if (read(sock, &file_num, sizeof(int)) == -1) {
                fprintf(stderr, "file number read failed");
                exit(-1);
            }

            int get_struct = 0;
            while (get_struct < sizeof(struct File) * file_num) {
                read_file = read(sock, ((char*)f) + get_struct, sizeof(struct File) * file_num - get_struct);
                if (read_file <= 0) {
                    fprintf(stderr, "file info read failed");
                    exit(-1);
                }
                get_struct += read_file;
            }
            printf("---------------------------------------------------------\n");
            for (int i = 0; i < file_num; i++) {
                printf("%d. %s (file size: %d bytes)\n", i + 1, f[i].fname, f[i].fsize);
            }
            printf("---------------------------------------------------------\n");
            int file_index, read_cnt;
            FILE *fp;
            char buf[BUF_SIZE];
            char dummy;
            while (1) {
                printf("Choose a number of file to get (Quit: 0): ");
                if (scanf("%d", &file_index) != 1) {
                    fprintf(stderr, "Invalid input\n");
                }

                // scanf를 사용한 후 개행 문자를 제거하기 위해 getchar를 사용합니다.
                dummy = getchar();

                if (write(sock, &file_index, sizeof(int)) == -1) {
                    fprintf(stderr, "file index write failed");
                    exit(-1);
                }

                if (file_index == 0) {
                    break;
                }

                int file_size;
                if (read(sock, &file_size, sizeof(int)) == -1) {
                    fprintf(stderr, "file size read failed\n");
                    exit(-1);
                }

                if (file_size == -1) {
                    printf("Failed to open file: %s\n", f[file_index - 1].fname);
                    continue;
                }

                fp = fopen(f[file_index - 1].fname, "wb");

                int total_received = 0;
                while (total_received < file_size) {
                    read_cnt = read(sock, buf, BUF_SIZE);
                    if (read_cnt == -1) {
                        fprintf(stderr, "file read failed\n");
                        exit(-1);
                    }
                    fwrite(buf, 1, read_cnt, fp);
                    total_received += read_cnt;
                }

                fclose(fp);
                printf("File downloaded: %s\n", f[file_index - 1].fname);
            }
        }

        if (strncmp(message, "ul", 2) == 0) {
            char file_name[BUF_SIZE];
            printf("Enter the file name to upload: ");
            fgets(file_name, BUF_SIZE, stdin);
            file_name[strcspn(file_name, "\n")] = '\0';  // Remove the newline character
            cmd_ul(sock, file_name);
        }
    }
    
    close(sock);
    return 0;
}

void cmd_ls(int sock, char *message) {
    int file_num, read_file;
    struct File f[20]; // File structure

    if (read(sock, &file_num, sizeof(int)) == -1) {
        fprintf(stderr, "file number read failed");
        exit(-1);
    }

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
        if (f[i].fname[strlen(f[i].fname) - 1] == '/') {
            printf("[directory] %s \n", f[i].fname);
        } else {
            printf("[file] %s (file size: %d bytes)\n", f[i].fname, f[i].fsize);
        }
    }
}

void cmd_ul(int sock, char *file_name) {
    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;

    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        perror("fopen() error");
        return;
    }

    // 파일 이름 전송
    if (write(sock, file_name, BUF_SIZE) <= 0) {
        perror("file name write error");
        fclose(fp);
        return;
    }

    // 파일 크기 계산
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("File size: %d bytes\n", file_size); // Debug print

    // 파일 크기 전송
    if (write(sock, &file_size, sizeof(int)) <= 0) {
        perror("file size write error");
        fclose(fp);
        return;
    }

    // 파일 내용 전송
    int total_sent = 0;
    while ((read_cnt = fread(buf, 1, BUF_SIZE, fp)) > 0) {
        int sent = 0;
        while (sent < read_cnt) {
            int n = write(sock, buf + sent, read_cnt - sent);
            if (n == -1) {
                perror("file content write error");
                fclose(fp);
                return;
            }
            sent += n;
        }
        total_sent += read_cnt;
    }

    fclose(fp);
    printf("File uploaded: %s (%d bytes)\n", file_name, file_size);
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
