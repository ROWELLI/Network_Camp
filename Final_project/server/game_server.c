#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define BUF_SIZE 30
#define KEY_BUF_SIZE 5

void error_handling(char *message);
void *handle_client(void *arg);

// 플레이어 정보
struct player {
    int player_id; // index
    int team; // red: 1, blue: 2
    int x;
    int y;
};

// 초반에 보내는 게임 정보
struct board_info {
    int player_number;
    int play_time;
    int room_width;
    int room_height;
    int tile_num;
};

// 클라이언트 정보를 스레드에 전달
struct t_arg {
    int clnt_sd;
    int client_id;
};

// 게임 정보
struct game_info {
    int board[BUF_SIZE][BUF_SIZE];
    int player_id;
};

void set_random_board(int size, int board_num, struct game_info *g_info);
void set_random_positions(int player_number, int size, struct player *players, struct game_info *g_info);

struct player players[BUF_SIZE];
struct board_info b_info;
struct game_info g_info; // 배열이 아닌 단일 구조체로 변경

int *clnt_sds; // 여러 클라이언트 정보 저장
pthread_mutex_t mutex;
int game_running = 1;

int main(int argc, char *argv[]) {
    int serv_sd, clnt_sd;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t *t_id;
    int n, s, b, t, port;
    int max_client = 0;

    if (argc != 11) {
        printf("Usage: %s -n <player_num> -s <size> -b <board_num> -p <port> -t <time>\n", argv[0]);
        exit(1);
    }

    n = atoi(argv[2]);
    s = atoi(argv[4]);
    b = atoi(argv[6]);
    t = atoi(argv[10]);
    port = atoi(argv[8]);

    // socket()
    serv_sd = socket(PF_INET, SOCK_STREAM, 0);   
    if (serv_sd == -1)
        error_handling("socket() error");
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(port);
    
    // bind()
    if (bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    //listen()
    if (listen(serv_sd, 5) == -1)
        error_handling("listen() error");

    clnt_sds = (int*)malloc(sizeof(int) * n);
    t_id = (pthread_t*)malloc(sizeof(pthread_t) * n);
    pthread_mutex_init(&mutex, NULL);

    clnt_adr_sz = sizeof(clnt_adr); 

    // n명 들어올 때까지 기다리기
    while (max_client < n) {
        printf("waiting for player...\n");
        clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if (clnt_sd == -1)
            error_handling("accept() error");
        
        printf("connected client: %d \n", clnt_sd);
        clnt_sds[max_client] = clnt_sd;

        // 클라이언트 ID 전송
        int client_id = max_client;
        int total = 0;
        int sent = 0;
        while (total < sizeof(client_id)) {
            sent = write(clnt_sd, ((char *)&client_id) + total, sizeof(client_id) - total);
            if (sent == -1)
                error_handling("write() error: client_id");
            total += sent;
        }

        max_client++;
    }

    // board info 설정
    b_info.room_width = s;
    b_info.room_height = s;
    b_info.player_number = n;
    b_info.play_time = t;
    b_info.tile_num = b;

    // player info 설정
    for (int i = 0; i < n; i++) {
        players[i].player_id = i; // index
        players[i].team = (i % 2 != 0) ? 1 : 2;
    }

    set_random_board(s, b, &g_info);
    set_random_positions(n, s, players, &g_info);

    // 클라이언트 별로 thread 생성
    for (int i = 0; i < n; i++) {
        struct t_arg *client_info = (struct t_arg*)malloc(sizeof(struct t_arg));
        client_info->clnt_sd = clnt_sds[i];
        client_info->client_id = i;

        pthread_create(&t_id[i], NULL, handle_client, (void*)client_info);
        pthread_detach(t_id[i]);
    }

    // 타이머 시작
    sleep(t);
    game_running = 0;

    // 게임 종료
    for (int i = 0; i < n; i++) {
        close(clnt_sds[i]);
    }

    close(serv_sd);
    free(clnt_sds);
    free(t_id);
    pthread_mutex_destroy(&mutex);
    printf("Game over!\n");
    return 0;
}

void *handle_client(void *arg) {
    struct t_arg *client_info = (struct t_arg*)arg;
    int clnt_sd = client_info->clnt_sd;
    int client_id = client_info->client_id;
    char key_buf[KEY_BUF_SIZE];
    int flag = 0; // 변경사항 플래그

    int x, y;

    // 데이터를 나누어 전송
    int total, sent;

    // board_info 전송
    total = 0;
    while (total < sizeof(b_info)) {
        sent = write(clnt_sd, ((char *)&b_info) + total, sizeof(b_info) - total);
        if (sent == -1) {
            error_handling("write() error: board_info");
        }
        total += sent;
    }

    // players 정보 전송
    total = 0;
    while (total < sizeof(players)) {
        sent = write(clnt_sd, ((char *)players) + total, sizeof(players) - total);
        if (sent == -1) {
            error_handling("write() error: players");
        }
        total += sent;
    }

    // game_info 전송
    total = 0;
    while (total < sizeof(struct game_info)) {
        sent = write(clnt_sd, ((char *)&g_info) + total, sizeof(struct game_info) - total);
        if (sent == -1) {
            error_handling("write() error: game_info");
        }
        total += sent;
    }

    while (game_running) {
        // 클라이언트 키 입력 읽기
        int str_len = read(clnt_sd, key_buf, sizeof(key_buf));
        if (str_len <= 0) break;

        // 키 입력 처리
        printf("Client %d pressed: %s\n", client_id, key_buf);

        // 동시성 문제 해결 위해 mutex 설정
        pthread_mutex_lock(&mutex);
        // 변화 확인
        flag = 0;
        // 위 방향키
        if (key_buf[0] == 'U' && players[client_id].y > 0) {
            players[client_id].y--;
            flag = 1;
        }
        // 아래 방향키 
        else if (key_buf[0] == 'D' && players[client_id].y < b_info.room_height - 1) {
            players[client_id].y++;
            flag = 1;
        } 
        // 왼쪽 방향키
        else if (key_buf[0] == 'L' && players[client_id].x > 0) {
            players[client_id].x--;
            flag = 1;
        } 
        // 오른쪽 방향키
        else if (key_buf[0] == 'R' && players[client_id].x < b_info.room_width - 1) {
            players[client_id].x++;
            flag = 1;
        } 
        // 엔터키
        else if (key_buf[0] == 'E') {
            x = players[client_id].x;
            y = players[client_id].y;
            if (g_info.board[y][x] == 1) {
                g_info.board[y][x] = 2; // 빨간색에서 파란색으로 변경
            } else if (g_info.board[y][x] == 2) {
                g_info.board[y][x] = 1; // 파란색에서 빨간색으로 변경
            }
            flag = 1;
        }
        pthread_mutex_unlock(&mutex);

        // 변경사항이 있는 경우 모든 클라이언트에게 브로드캐스트
        if (flag) {
            for (int i = 0; i < b_info.player_number; i++) {
                total = 0;
                while (total < sizeof(players)) {
                    sent = write(clnt_sds[i], ((char *)players) + total, sizeof(players) - total);
                    if (sent == -1) {
                        error_handling("write() error: players update");
                    }
                    total += sent;
                }

                total = 0;
                while (total < sizeof(struct game_info)) {
                    sent = write(clnt_sds[i], ((char *)&g_info) + total, sizeof(struct game_info) - total);
                    if (sent == -1) {
                        error_handling("write() error: game_info update");
                    }
                    total += sent;
                }
            }
        }
    }

    close(clnt_sd); 
    free(client_info);
    return NULL;
}

// 초기 랜덤 보드 세팅
void set_random_board(int size, int board_num, struct game_info *g_info) {
    srand(time(NULL));
    int count1 = 0, count2 = 0;

    // 초기화
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            g_info->board[i][j] = 0;
        }
    }

    while (count1 < board_num/2 || count2 < board_num/2) {
        int x = rand() % size;
        int y = rand() % size;

        if (g_info->board[x][y] == 0) {
            if (count1 < board_num/2) {
                g_info->board[x][y] = 1;
                count1++;
            } else if (count2 < board_num/2) {
                g_info->board[x][y] = 2;
                count2++;
            }
        }
    }
}

// 초기 랜덤 포지션 세팅
void set_random_positions(int player_number, int size, struct player *players, struct game_info *g_info) {
    srand(time(NULL));
    int occupied[BUF_SIZE][BUF_SIZE] = {0}; // 중복 위치 방지를 위한 보드

    for (int i = 0; i < player_number; i++) {
        int x, y;
        do {
            x = rand() % size;
            y = rand() % size;
        } while (occupied[x][y]); // 이미 차지된 위치인지 확인

        players[i].x = x;
        players[i].y = y;
        occupied[x][y] = 1; // 위치 차지 표시
    }
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
