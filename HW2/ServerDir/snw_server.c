#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#define NAME_SIZE 100
#define BUF_SIZE 1000

void error_handling(char *message);
void sig_handler(int signo);

typedef struct {
    char buf[BUF_SIZE];
    int pkt;
    int length;
} packet;

int main(int argc, char *argv[])
{
    char filename[NAME_SIZE];
    int str_len, get_ack;
    FILE* file;
    packet pkt_data;
    int sendlength;
    struct stat sb;
    int ACK;
    int pkt = 0;

    int serv_sock;
    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz;
    struct sockaddr_in serv_adr;

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("UDP socket creation error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    // 파일 이름 받기
    clnt_adr_sz = sizeof(clnt_adr);
    str_len = recvfrom(serv_sock, filename, NAME_SIZE, 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
    filename[str_len] = '\0';
    printf("Requested file: %s\n", filename);

    // 파일 열기
    if((file = fopen(filename, "rb")) == NULL) {
        error_handling("File open error");
    }

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        error_handling("sigaction error");
    }

    while (1) {
        // 패킷 데이터 읽기
        sendlength = fread(pkt_data.buf, 1, BUF_SIZE, file);
        if (sendlength <= 0) {
            strcpy(pkt_data.buf, "eof");
            pkt_data.pkt = pkt;
            pkt_data.length = 0;
            sendto(serv_sock, &pkt_data, sizeof(packet), 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz);
            break;
        }
        pkt_data.pkt = pkt;
        pkt_data.length = sendlength;

        int ack_received = 0;
        while (!ack_received) {
            // 패킷 전송
            if (sendto(serv_sock, &pkt_data, sizeof(pkt_data), 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz) == -1) {
                error_handling("file send failed");
            }
            // printf("Sent packet %d, size: %d bytes\n", pkt, sendlength);

            // ACK 메시지 대기
            alarm(2);
            get_ack = recvfrom(serv_sock, &ACK, sizeof(int), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
            alarm(0);

            if (get_ack == -1) {
                if (errno == EINTR) {
                    printf("resending packet %d\n", pkt);
                    continue; 
                } 
                else {
                    error_handling("recvfrom error");
                }
            }

            if (ACK == pkt) {
                // printf("Received ACK for packet %d\n", pkt);
                ack_received = 1;
                pkt++;
            } 
        }
    }

    fclose(file);
    close(serv_sock);
    printf("File transfer complete\n");
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void sig_handler(int signo){
    printf("Timeout!\n");
}
