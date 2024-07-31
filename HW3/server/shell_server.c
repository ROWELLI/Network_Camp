#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 10

void error_handling(char *buf);
void cmd_ls(int clnt_sock, char *cur_dir);
void cmd_cd(int clnt_sock, char *cur_dir, char *dir_name);
void cmd_ul(int clnt_sock, char *cur_dir);
void cmd_dl(int clnt_sock, char *cur_dir);
void *handle_client_cmd(void *arg);

struct File {
    char fname[256];
    int fsize;
};

struct Client {
    int sock;
    char cur_dir[BUF_SIZE];
    char server_dir[BUF_SIZE];
};

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    char server_dir[BUF_SIZE]; 
    pthread_t t_id;

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

    // 서버 디렉토리 가져오기
    if (getcwd(server_dir, BUF_SIZE) == NULL) {
        error_handling("getcwd() error");
    }
    // printf("Server directory: %s\n", server_dir);

    while (1)
    {
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
        printf("connected client: %d \n", clnt_sock);

        // client 초기화
        struct Client *client_info = malloc(sizeof(struct Client));
        client_info->sock = clnt_sock;
        strncpy(client_info->cur_dir, server_dir, BUF_SIZE);
        strncpy(client_info->server_dir, server_dir, BUF_SIZE);

        // thread 생성
        pthread_create(&t_id, NULL, handle_client_cmd, (void*)client_info);
        pthread_detach(t_id);
    }

    close(serv_sock);
    return 0;
}

// 클라이언트 cmd 처리 
void *handle_client_cmd(void *arg) {
    struct Client *client_info = (struct Client*)arg;
    int clnt_sock = client_info->sock;
    char buf[BUF_SIZE];
    int str_len;

    while ((str_len = read(clnt_sock, buf, BUF_SIZE - 1)) != 0) {
        if (str_len == -1) {
            error_handling("read() error");
        }

        // printf("Received message: %s\n", buf); 

        //  파일 list
        if (strncmp(buf, "ls", 2) == 0) {
            cmd_ls(clnt_sock, client_info->cur_dir);
        } 
        // 디렉토리 이동
        else if (strncmp(buf, "cd", 2) == 0) {
            char dir_name[BUF_SIZE] = {0};
            strncpy(dir_name, buf + 3, BUF_SIZE - 4);
            dir_name[strlen(dir_name) - 1] = '\0'; 
            cmd_cd(clnt_sock, client_info->cur_dir, dir_name);  
        }
        // 파일 다운로드
        else if (strncmp(buf, "dl", 2) == 0){
            cmd_dl(clnt_sock, client_info->cur_dir); 
        }
        // 파일 업로드
        else if (strncmp(buf, "ul", 2) == 0){
            cmd_ul(clnt_sock, client_info->cur_dir); 
        }
    }

    close(clnt_sock);
    free(client_info);
    return NULL;
}

void cmd_ls(int clnt_sock, char *cur_dir) {
    DIR *dp;
    struct dirent *dir;
    struct File f[20]; // File structure
    struct stat sb; // include stat

    // 현재 디렉토리 열기
    if ((dp = opendir(cur_dir)) == NULL) {
        error_handling("opendir error");
    }

    // 파일 이름과 크기 가져오기
    int file_num = 0;
    while ((dir = readdir(dp)) != NULL) {
        if (dir->d_ino == 0) continue;
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue; // readdir has ., .. because of the directory place
        char filepath[BUF_SIZE];
        strcpy(filepath, cur_dir);
        strcat(filepath, "/");
        strcat(filepath, dir->d_name); 
        if (stat(filepath, &sb) == -1) {
            error_handling("stat error");
        }

        if (S_ISDIR(sb.st_mode)) {
            strcpy(f[file_num].fname, dir->d_name);
            strcat(f[file_num].fname, "/"); // 디렉토리 이름 뒤에 / 추가
        } else {
            strncpy(f[file_num].fname, dir->d_name, sizeof(f[file_num].fname) - 1);
            f[file_num].fname[sizeof(f[file_num].fname) - 1] = '\0';
        }
        f[file_num].fsize = sb.st_size;
        file_num++;
    }
    closedir(dp);  

    // 파일 이름 전송
    if (write(clnt_sock, &file_num, sizeof(int)) == -1) {
        error_handling("file number write error");
    }
    // 파일 구조체 전송
    if (write(clnt_sock, f, sizeof(struct File) * file_num) == -1) {
        error_handling("file info write error");
    }
}


void cmd_cd(int clnt_sock, char *cur_dir, char *dir_name) {
    dir_name[strcspn(dir_name, "\r\n")] = 0; // '\r'이나 '\n' 문자를 찾아서 그 위치를 문자열의 끝으로 설정

    char new_dir[BUF_SIZE];
    if (strcmp(dir_name, "..") == 0) {
        if (chdir("..") == -1) {
            error_handling("chdir() error");
        }
    } else {
        strcpy(new_dir, cur_dir);
        strcat(new_dir, "/");
        strcat(new_dir, dir_name);
        if (chdir(new_dir) == -1) {
            error_handling("chdir() error");
        }
    }

    // 현재 디렉토리 경로 업데이트
    if (getcwd(cur_dir, BUF_SIZE) == NULL) {
        error_handling("getcwd() error");
    }

    printf("Directory changed to: %s\n", cur_dir);
}

void cmd_ul(int clnt_sock, char *cur_dir) {
    char file_name[BUF_SIZE];
    int file_size;
    char buf[BUF_SIZE];
    int read_cnt;
    FILE *fp;

    // 파일 이름 읽기
    if (read(clnt_sock, file_name, BUF_SIZE) <= 0) {
        error_handling("file name read error");
    }
    printf("Uploading file: %s\n", file_name);

    // 파일 크기 읽기
    if (read(clnt_sock, &file_size, sizeof(int)) <= 0) {
        error_handling("file size read error");
    }
    // printf("File size: %d bytes\n", file_size);

    // 파일 열기
    char file_path[BUF_SIZE];
    strcpy(file_path, cur_dir);
    strcat(file_path, "/");
    strcat(file_path, file_name);
    fp = fopen(file_path, "wb");

    // 파일 내용 읽기
    int total_received = 0;
    while (total_received < file_size) {
        read_cnt = read(clnt_sock, buf, BUF_SIZE);
        if (read_cnt <= 0) {
            error_handling("read_cnt error");
            break;
        }
        fwrite(buf, 1, read_cnt, fp);
        total_received += read_cnt;
    }

    fclose(fp);
    printf("File uploaded: %s\n", file_name);
}

void cmd_dl(int clnt_sock, char *cur_dir) {
    DIR *dp;
    struct dirent *dir;
    struct File f[20]; 
    struct stat sb; 

    // 현재 디렉토리 열기
    if ((dp = opendir(cur_dir)) == NULL) {
        error_handling("opendir() error");
    }

    // 파일 이름과 크기 가져오기
    int file_num = 0;
    while ((dir = readdir(dp)) != NULL) {
        if (dir->d_ino == 0) continue;
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue; // readdir has ., .. because of the directory place

        // stat 사용
        char filepath[BUF_SIZE];
        strcpy(filepath, cur_dir);
        strcat(filepath, "/");
        strcat(filepath, dir->d_name);  
        if (stat(filepath, &sb) == -1) {
            error_handling("stat() error");
        }

        // 디렉토리가 아닌 경우에만 리스트에 추가
        if (S_ISDIR(sb.st_mode)) continue;

        strcpy(f[file_num].fname, dir->d_name);
        f[file_num].fname[sizeof(f[file_num].fname) - 1] = '\0';
        f[file_num].fsize = sb.st_size;
        file_num++;
    }
    closedir(dp);

    if (write(clnt_sock, &file_num, sizeof(int)) == -1) {
        error_handling("file num write error");
    }

    if (write(clnt_sock, f, sizeof(struct File) * file_num) == -1) {
        error_handling("file info write error");
    }

    int file_index, read_cnt;
    FILE *fp;
    char buf[BUF_SIZE];

    while (1) {
        if (read(clnt_sock, &file_index, sizeof(int)) == -1) {
            error_handling("file index read error");
        }

        if (file_index == 0) break;

        if (file_index - 1 < 0 || file_index - 1 >= file_num) {
            error_handling("file index out of range");
            continue;
        }

        char filepath[BUF_SIZE];
        strcpy(filepath, cur_dir);
        strcat(filepath, "/");
        strcat(filepath, f[file_index - 1].fname); 
        fp = fopen(filepath, "rb");
        if (fp == NULL) {
            // 파일 열기 오류를 클라이언트에 전송
            int error_code = -1;
            if (write(clnt_sock, &error_code, sizeof(int)) == -1) {
                error_handling("message send error");
            }
            continue;
        }

        // 파일 전송
        int file_size = f[file_index - 1].fsize;
        if (write(clnt_sock, &file_size, sizeof(int)) == -1) {
            error_handling("file size send error");
        }

        int total_sent = 0;
        while ((read_cnt = fread((void*)buf, 1, BUF_SIZE, fp)) > 0) {
            int sent = 0;
            while (sent < read_cnt) {
                int n = write(clnt_sock, buf + sent, read_cnt - sent);
                if (n == -1) {
                    error_handling("file send error");
                    fclose(fp);
                    return;
                }
                sent += n;
            }
            total_sent += read_cnt;
        }
        fclose(fp);
    }
}



void error_handling(char *buf)
{
    fputs(buf, stderr);
    fputc('\n', stderr);
    exit(1);
}
