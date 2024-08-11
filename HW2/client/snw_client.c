#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define NAME_SIZE 100
#define BUF_SIZE 1000

void error_handling(char *message);

typedef struct {
    char buf[BUF_SIZE];
    int pkt;
    int length;
} packet;

int main(int argc, char *argv[])
{
    int sock;
    char message[NAME_SIZE];
    packet pkt_data;
    int str_len;
    long file_size = 0;
    socklen_t adr_sz;
    FILE* file;
    int ACK;
    int expected_pkt = 0;

    struct sockaddr_in serv_adr, from_adr;
    if (argc != 4) {
        printf("Usage : %s <IP> <port> <filename>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);   
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    // argv[3]에 저장된 파일 이름 보내기
    strcpy(message, argv[3]);
    sendto(sock, message, strlen(message), 0, 
                (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    // 파일 쓰기 준비
    file = fopen(argv[3], "wb");
    if (file == NULL) {
        error_handling("File open error");
    }

    // 랜덤 함수 초기화
    srand(time(NULL));

    // 파일 내용 받기
    while (1) {
        adr_sz = sizeof(from_adr);
        str_len = recvfrom(sock, &pkt_data, sizeof(packet), 0, (struct sockaddr*)&from_adr, &adr_sz);
        if (str_len <= 0) {
            error_handling("recvfrom error");
        }

        // 1% 확률로 패킷 손실 
        if (rand() % 100 < 1) {
            printf("packet loss %d\n", pkt_data.pkt);
            continue; 
        }

        if (strcmp(pkt_data.buf, "eof") == 0) {
            break;
        }

        if (pkt_data.pkt == expected_pkt) {
            fwrite(pkt_data.buf, 1, pkt_data.length, file);
            file_size += pkt_data.length;
            // printf("Received packet %d, size: %d bytes\n", pkt_data.pkt, pkt_data.length);

            // ACK 보내기
            ACK = pkt_data.pkt;
            sendto(sock, &ACK, sizeof(int), 0, (struct sockaddr*)&from_adr, adr_sz);
            expected_pkt++;
        }
    }

    fclose(file);
    close(sock);
    printf("File received and saved\n");
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
