#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 100
#define MAX_CLIENTS 10

void error_handling(char *buf);
void cmd_ls(int clnt_sock, char *cur_dir);
void cmd_cd(int clnt_sock, char *cur_dir, char *dir_name);

// ls
struct File {
    char fname[256];
    int fsize;
};

struct ClientInfo {
    int sock;
    char cur_dir[BUF_SIZE];
};

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct timeval timeout;
    fd_set reads, cpy_reads;

    struct ClientInfo clients[MAX_CLIENTS];
    int client_count = 0;

    socklen_t adr_sz;
    int fd_max, str_len, fd_num, i;
    char buf[BUF_SIZE];

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    
    if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    while (1)
    {
        cpy_reads = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 5000;

        if ((fd_num = select(fd_max+1, &cpy_reads, 0, 0, &timeout)) == -1)
            break;
        
        if (fd_num == 0)
            continue;

        for (i = 0; i < fd_max + 1; i++)
        {
            if (FD_ISSET(i, &cpy_reads))
            {
                if (i == serv_sock)     // connection request!
                {
                    adr_sz = sizeof(clnt_adr);
                    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                    FD_SET(clnt_sock, &reads);
                    if (fd_max < clnt_sock)
                        fd_max = clnt_sock;
                    printf("connected client: %d \n", clnt_sock);

                    // Initialize client information
                    clients[client_count].sock = clnt_sock;
                    getcwd(clients[client_count].cur_dir, BUF_SIZE);
                    client_count++;
                }
                else    // read message!
                {
                    str_len = read(i, buf, BUF_SIZE - 1);
                    if (str_len == 0)    // close request!
                    {
                        FD_CLR(i, &reads);
                        close(i);
                        printf("closed client: %d \n", i);
                        
                        // Remove client information
                        for (int j = 0; j < client_count; j++) {
                            if (clients[j].sock == i) {
                                for (int k = j; k < client_count - 1; k++) {
                                    clients[k] = clients[k + 1];
                                }
                                client_count--;
                                break;
                            }
                        }
                    }
                    else
                    {
                        buf[str_len] = '\0'; // Ensure null-terminated string
                        printf("Received message: %s\n", buf); // Debug print

                        // Find the client information
                        struct ClientInfo *client_info = NULL;
                        for (int j = 0; j < client_count; j++) {
                            if (clients[j].sock == i) {
                                client_info = &clients[j];
                                break;
                            }
                        }

                        if (client_info != NULL) {
                            if (strncmp(buf, "ls", 2) == 0) {
                                cmd_ls(i, client_info->cur_dir);
                            } 
                            else if (strncmp(buf, "cd", 2) == 0) {
                                char dir_name[BUF_SIZE] = {0};
                                strncpy(dir_name, buf + 3, BUF_SIZE - 4);
                                dir_name[strlen(dir_name) - 1] = '\0'; // Remove newline character
                                printf("Changing directory to: %s\n", dir_name); // Debug print
                                cmd_cd(i, client_info->cur_dir, dir_name);
                            }
                        }
                    }
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}

void cmd_ls(int clnt_sock, char *cur_dir) {
    DIR *dp;
    struct dirent *dir;
    struct File f[20]; // File structure
    struct stat sb; // include stat

    // open directory
    if ((dp = opendir(cur_dir)) == NULL) {
        fprintf(stderr, "directory open error\n");
        exit(-1);
    }

    // get file name and size
    int file_num = 0;
    printf("filename and size to send\n");
    while ((dir = readdir(dp)) != NULL) {
        if (dir->d_ino == 0) continue;
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue; // readdir has ., .. because of the directory place
        strcpy(f[file_num].fname, dir->d_name);
        f[file_num].fname[sizeof(f[file_num].fname) - 1] = '\0';

        // use stat
        char filepath[BUF_SIZE];
        snprintf(filepath, BUF_SIZE, "%s/%s", cur_dir, f[file_num].fname);
        if (stat(filepath, &sb) == -1) {
            fprintf(stderr, "stat error");
            exit(-1);
        }
        f[file_num].fsize = sb.st_size;
        printf("file name: %s, file size: %d\n", f[file_num].fname, f[file_num].fsize);
        file_num++;
    }
    if (write(clnt_sock, &file_num, sizeof(int)) == -1) {
        fprintf(stderr, "file number write failed");
        exit(-1);
    }

    if (write(clnt_sock, f, sizeof(struct File) * file_num) == -1) {
        fprintf(stderr, "file info write failed");
        exit(1);
    }
    closedir(dp);
}

void cmd_cd(int clnt_sock, char *cur_dir, char *dir_name) {
    char new_dir[BUF_SIZE];
    snprintf(new_dir, BUF_SIZE, "%s/%s", cur_dir, dir_name);

    if (chdir(new_dir) == -1) {
        perror("chdir() error");
        return;
    }

    if (getcwd(cur_dir, BUF_SIZE) == NULL) {
        perror("getcwd() error");
        return;
    }

    printf("Directory changed to: %s\n", cur_dir);
}

void error_handling(char *buf)
{
    fputs(buf, stderr);
    fputc('\n', stderr);
    exit(1);
}
