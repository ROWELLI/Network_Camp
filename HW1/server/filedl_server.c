#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 2048

void error_handling(char *message);

struct File {
    char fname[256];
    int fsize;
};

int main(int argc, char *argv[]) {
    int serv_sock; // file descriptor for the socket
    int clnt_sock;
    struct sockaddr_in serv_addr; // address (IP address and port number) to assign a socket
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    int file_index, read_cnt;
    FILE *fp;
    char buf[BUF_SIZE];

    struct stat sb; // include stat

    DIR *dp;
    struct dirent *dir;
    struct File f[20]; // File structure

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // socket()
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // bind()
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    // open directory
    if ((dp = opendir(".")) == NULL) {
        fprintf(stderr, "directory open error\n");
        exit(-1);
    }

    int i = 0;
    // get file name and size
    printf("filename and size to sent\n");
    while ((dir = readdir(dp)) != NULL) {
        if (dir->d_ino == 0) continue;
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue; // readdir has ., .. because of the directory place
        strcpy(f[i].fname, dir->d_name);
        f[i].fname[sizeof(f[i].fname) - 1] = '\0';

        // use stat
        if (stat(f[i].fname, &sb) == -1) {
            fprintf(stderr, "stat error");
            exit(-1);
        }
        f[i].fsize = sb.st_size;
        printf("file name: %s, file size: %d\n", f[i].fname, f[i].fsize);
        i++;
    }

    closedir(dp);
    // accept
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1)
        error_handling("accept() error");

    if (write(clnt_sock, &i, sizeof(int)) == -1) {
        fprintf(stderr, "file number write failed");
        exit(-1);
    }

    if (write(clnt_sock, f, sizeof(struct File) * i) == -1) {
        fprintf(stderr, "file info write failed");
        return 1;
    }

    while (1) {
        if (read(clnt_sock, &file_index, sizeof(int)) == -1) {
            fprintf(stderr, "file index read failed");
            exit(-1);
        }

        if (file_index == 0) break;

        printf("index of file to get: %d\n", file_index);
        fp = fopen(f[file_index - 1].fname, "rb");

        // send file content
        int total_sent = 0;
        while ((read_cnt = fread((void*)buf, 1, BUF_SIZE, fp)) > 0) {
            if (write(clnt_sock, buf, read_cnt) != read_cnt) {
                fprintf(stderr, "file send failed\n");
                exit(-1);
            }
            total_sent += read_cnt;
        }   
        fclose(fp);
    }

    close(clnt_sock);
    close(serv_sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
