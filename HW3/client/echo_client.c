#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char *message);
void cmd_ls(int sock, char *message);

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
        fputs("Input message(Q to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);
        int str_len = write(sock, message, strlen(message));
        
        if (!strcmp(message,"q\n") || !strcmp(message,"Q\n"))
            break;

        if (strncmp(message, "ls", 2) == 0){
            cmd_ls(sock, message);
        }
        // char dir_name[BUF_SIZE]; 
        // if(strncmp(message, "cd", 2) == 0){
        //     
        // }
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
        printf("%d. file name: %s (file size: %d bytes)\n", i + 1, f[i].fname, f[i].fsize);
    }
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
