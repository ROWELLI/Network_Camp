#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>
#include <pthread.h>

#define BUF_SIZE 30
#define KEY_BUF_SIZE 5

void error_handling(char *message);

struct player {
    int player_id; // index
    int team; // red: 1, blue: 2
    int x;
    int y;
};

struct board_info {
    int player_number;
    int play_time;
    int room_width;
    int room_height;
    int tile_num;
};

struct game_info {
    int board[BUF_SIZE][BUF_SIZE];
    int player_id;
};

void display_board(struct game_info *g_info, int cell_width, int cell_height, struct player *players, int client_id);
void *update_state(void *arg);
void *timer(void *arg);
void display_info(struct player *p, int client_id);
void display_result();

struct board_info b_info;
struct game_info g_info;
int cell_width, cell_height, client_id;
int game_running = 1;
int left_time;

int main(int argc, char *argv[]) {
    int sd;
    struct sockaddr_in serv_adr;
    struct player p[BUF_SIZE];

    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    // connect()
    if (connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error");

    // 클라이언트 ID 수신
    int total = 0;
    int received = 0;
    while (total < sizeof(client_id)) {
        received = read(sd, ((char *)&client_id) + total, sizeof(client_id) - total);
        if (received == -1)
            error_handling("read() error: client_id");
        total += received;
    }

    printf("Connected as client number: %d\n", client_id);

    // board_info 수신
    total = 0;
    while (total < sizeof(b_info)) {
        received = read(sd, ((char *)&b_info) + total, sizeof(b_info) - total);
        if (received == -1)
            error_handling("read() error: board_info");
        total += received;
    }

    // players 정보 수신
    total = 0;
    while (total < sizeof(p)) {
        received = read(sd, ((char *)p) + total, sizeof(p) - total);
        if (received == -1)
            error_handling("read() error: players");
        total += received;
    }

    // game_info 수신
    total = 0;
    while (total < sizeof(g_info)) {
        received = read(sd, ((char *)&g_info) + total, sizeof(g_info) - total);
        if (received == -1)
            error_handling("read() error: game_info");
        total += received;
    }

    // 타이머 설정
    left_time = b_info.play_time;

    // ncurses 초기화
    initscr();
    start_color();
    init_pair(1, COLOR_RED, COLOR_RED);
    init_pair(2, COLOR_BLUE, COLOR_BLUE);
    init_pair(3, COLOR_WHITE, COLOR_WHITE);
    init_pair(4, COLOR_CYAN, COLOR_CYAN);   // 하늘색
    init_pair(5, COLOR_YELLOW, COLOR_YELLOW); // 주황색
    init_pair(6, COLOR_GREEN, COLOR_GREEN); // 연두색

    // 정보 표시
    display_info(p, client_id);

    // 터미널 크기 가져오기
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width);

    // 보드 크기 계산
    cell_width = term_width / b_info.room_width;
    cell_height = (term_height - 12) / b_info.room_height; // 12 줄은 정보 및 타이머 출력용

    // 게임 보드 표시
    display_board(&g_info, cell_width, cell_height, p, client_id);

    refresh();

    // 타이머 스레드 시작
    pthread_t timer_thread;
    pthread_create(&timer_thread, NULL, timer, NULL);
    pthread_detach(timer_thread);

    // 키 입력 처리
    char key_buf[KEY_BUF_SIZE];
    keypad(stdscr, TRUE); // 방향키 입력을 받기 위해 keypad 활성화

    // 서버에서 업데이트된 정보를 읽기 스레드 생성
    pthread_t update_thread;
    pthread_create(&update_thread, NULL, update_state, &sd);
    pthread_detach(update_thread);

    while (game_running) {
        int ch = getch();
        memset(key_buf, 0, KEY_BUF_SIZE);
        if (ch == KEY_UP) {
            key_buf[0] = 'U';
        } 
        else if (ch == KEY_DOWN) {
            key_buf[0] = 'D';
        } 
        else if (ch == KEY_LEFT) {
            key_buf[0] = 'L';
        } 
        else if (ch == KEY_RIGHT) {
            key_buf[0] = 'R';
        } 
        else if (ch == '\n') {
            key_buf[0] = 'E'; 
        } 
        else {
            continue; 
        }

        // key 보내기
        int total = 0;
        int sent = 0;
        while (total < sizeof(key_buf)) {
            sent = write(sd, key_buf + total, sizeof(key_buf) - total);
            if (sent == -1) {
                error_handling("write() error: key_buf");
            }
            total += sent;
        }
    }

    display_result();

    endwin();

    close(sd);
    return 0;
}

// 서버에서 변한 상태 정보 업데이트
void *update_state(void *arg) {
    int sd = *(int *)arg;
    struct player p[BUF_SIZE];
    int total = 0;
    int received = 0;

    while (game_running) {

        // 위치 정보 업데이트
        total = 0;
        while (total < sizeof(p)) {
            received = read(sd, ((char *)p) + total, sizeof(p) - total);
            if (received <= 0) {
                game_running = 0;
                break;
            }
            total += received;
        }

        // 게임 정보 업데이트(색상 변화 여부)
        total = 0;
        while (total < sizeof(g_info)) {
            received = read(sd, ((char *)&g_info) + total, sizeof(g_info) - total);
            if (received <= 0) {
                game_running = 0;
                break;
            }
            total += received;
        }

        // 화면 상태
        clear();
        display_info(p, client_id);
        display_board(&g_info, cell_width, cell_height, p, client_id);
        mvprintw(10 + b_info.room_height * cell_height, 0, "Time remaining: %d seconds", left_time); // 타이머 갱신
        refresh();
    }

    return NULL;
}

// 타이머 설정
void *timer(void *arg) {
    while (left_time > 0 && game_running) {
        mvprintw(10 + b_info.room_height * cell_height, 0, "Time remaining: %d seconds", left_time);
        refresh();
        sleep(1);
        left_time--;
    }
    game_running = 0;
    return NULL;
}

// 게임 정보 표시
void display_info(struct player *p, int client_id) {
    mvprintw(0, 0, "Board Information:");
    mvprintw(1, 0, "Player Number: %d", b_info.player_number);
    mvprintw(2, 0, "Play Time: %d", b_info.play_time);
    mvprintw(3, 0, "Room Width: %d", b_info.room_width);
    mvprintw(4, 0, "Room Height: %d", b_info.room_height);
    mvprintw(5, 0, "Tile Number: %d", b_info.tile_num);

    // 수신한 players 정보 출력
    mvprintw(7, 0, "Player Information:");
    for (int i = 0; i < b_info.player_number; i++) {
        if (p[i].player_id == client_id) { // 내 정보 표시
            mvprintw(8 + i, 0, "==> Your Information: Player ID: %d, Team: %s, Position: (%d, %d) <==", 
                     p[i].player_id, (p[i].team == 1) ? "Red" : "Blue", p[i].x, p[i].y);
        } else {
            mvprintw(8 + i, 0, "Player ID: %d, Team: %s, Position: (%d, %d)", p[i].player_id, 
                     (p[i].team == 1) ? "Red" : "Blue", p[i].x, p[i].y);
        }
    }
}

// 결과 표시
void display_result() {
    int red_count = 0;
    int blue_count = 0;

    for (int i = 0; i < b_info.room_height; i++) {
        for (int j = 0; j < b_info.room_width; j++) {
            if (g_info.board[i][j] == 1) {
                red_count++;
            } else if (g_info.board[i][j] == 2) {
                blue_count++;
            }
        }
    }

    clear();
    if (red_count > blue_count) {
        mvprintw(0, 0, "Red team wins!");
    } else if (blue_count > red_count) {
        mvprintw(0, 0, "Blue team wins!");
    } else {
        mvprintw(0, 0, "It's a draw!");
    }

    mvprintw(2, 0, "Red tiles: %d", red_count);
    mvprintw(3, 0, "Blue tiles: %d", blue_count);

    refresh();
    sleep(5); // 결과를 5초 동안 표시
}

// 게임 판 표시
void display_board(struct game_info *g_info, int cell_width, int cell_height, struct player *players, int client_id) {
    for (int i = 0; i < b_info.room_height; i++) {
        for (int j = 0; j < b_info.room_width; j++) {
            for (int y = 0; y < cell_height; y++) {
                for (int x = 0; x < cell_width; x++) {
                    if (g_info->board[i][j] == 1) {
                        attron(COLOR_PAIR(1));
                        mvprintw(10 + i * cell_height + y, j * cell_width + x, " ");
                        attroff(COLOR_PAIR(1));
                    } else if (g_info->board[i][j] == 2) {
                        attron(COLOR_PAIR(2));
                        mvprintw(10 + i * cell_height + y, j * cell_width + x, " ");
                        attroff(COLOR_PAIR(2));
                    } else {
                        attron(COLOR_PAIR(3));
                        mvprintw(10 + i * cell_height + y, j * cell_width + x, " ");
                        attroff(COLOR_PAIR(3));
                    }
                }
            }
        }
    }

    // 플레이어 위치 표시
    for (int i = 0; i < b_info.player_number; i++) {
        if (players[i].player_id == client_id) { // 사용자 플레이어
            attron(COLOR_PAIR(6));
            mvprintw(10 + players[i].y * cell_height, players[i].x * cell_width, "P");
            attroff(COLOR_PAIR(6));
        } else if (players[i].team == 1) { // 빨간 팀
            attron(COLOR_PAIR(5));
            mvprintw(10 + players[i].y * cell_height, players[i].x * cell_width, "P");
            attroff(COLOR_PAIR(5));
        } else { // 파란 팀
            attron(COLOR_PAIR(4));
            mvprintw(10 + players[i].y * cell_height, players[i].x * cell_width, "P");
            attroff(COLOR_PAIR(4));
        }
    }
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
