#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define WIDTH 40
#define HEIGHT 20

char map[HEIGHT][WIDTH + 1];
int pacman_x, pacman_y; // Позиция Пакмана
int score = 0;
int total_points = 0;
int auto_play = 0; // Флаг для автопрохождения
int noclip = 0; // Флаг для режима noclip
char input_code[5]; // Массив для ввода кода
int input_index = 0;
const char *correct_code = "8184";

// Объявление функции draw_map
void draw_map();

void handle_resize(int sig) {
    // Обработка сигнала изменения размера окна
    endwin();
    refresh();
    clear();
    draw_map();
}

void generate_map() {
    srand(time(NULL));
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (y == 0 || y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {
                map[y][x] = '#'; // Границы карты
            } else {
                if (rand() % 4 == 0) {
                    map[y][x] = '#'; // Стены
                } else {
                    map[y][x] = '.'; // Точки
                    total_points++;
                }
            }
        }
        map[y][WIDTH] = '\0';
    }
    pacman_x = 1;
    pacman_y = 1;
    map[pacman_y][pacman_x] = ' '; // Начальная позиция Пакмана
}

void draw_map() {
    clear();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == pacman_x && y == pacman_y) {
                mvaddch(y, x, 'C'); // Отображение Пакмана
            } else {
                mvaddch(y, x, map[y][x]);
            }
        }
    }
    mvprintw(HEIGHT, 0, "Score: %d/%d", score, total_points); // Отображение очков
    mvprintw(HEIGHT + 1, 0, "Auto Play: %s | Noclip: %s", auto_play ? "ON" : "OFF", noclip ? "ON" : "OFF");
    mvprintw(HEIGHT + 2, 0, "Enter code to win: %s", input_code); // Отображение ввода кода
    refresh();
}

void move_pacman(int dx, int dy) {
    int new_x = pacman_x + dx;
    int new_y = pacman_y + dy;

    if (noclip || map[new_y][new_x] != '#') { // Проверяем, чтобы Пакман не врезался в стену
        pacman_x = new_x;
        pacman_y = new_y;

        if (map[new_y][new_x] == '.') { // Если Пакман съел точку
            map[new_y][new_x] = ' ';
            score++;

            if (score == total_points) { // Все точки собраны
                clear();
                mvprintw(HEIGHT / 2, (WIDTH / 2) - 5, "You Win!");
                mvprintw((HEIGHT / 2) + 1, (WIDTH / 2) - 10, "Final Score: %d/%d", score, total_points);
                refresh();
                getch();
                endwin();
                exit(0);
            }
        }
    }
}

void auto_move() {
    int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}}; // w, s, a, d
    int best_dir = -1;
    int max_points = -1;

    for (int i = 0; i < 4; i++) {
        int new_x = pacman_x + directions[i][0];
        int new_y = pacman_y + directions[i][1];
        int points = 0;

        if (map[new_y][new_x] != '#') {
            // Проверяем наличие точек рядом с новой позицией
            for (int j = 0; j < 4; j++) {
                int check_x = new_x + directions[j][0];
                int check_y = new_y + directions[j][1];
                if (map[check_y][check_x] == '.') {
                    points++;
                }
            }
            if (points > max_points) {
                max_points = points;
                best_dir = i;
            }
        }
    }

    if (best_dir != -1) {
        move_pacman(directions[best_dir][0], directions[best_dir][1]);
    } else {
        // Если нет точек поблизости, двигаться в случайном направлении, чтобы избежать застревания
        best_dir = rand() % 4;
        move_pacman(directions[best_dir][0], directions[best_dir][1]);
    }
}

void check_code() {
    if (strcmp(input_code, correct_code) == 0) {
        clear();
        mvprintw(HEIGHT / 2, (WIDTH / 2) - 5, "You Win!");
        mvprintw((HEIGHT / 2) + 1, (WIDTH / 2) - 12, "You entered the correct code: %s", input_code);
        refresh();
        getch();
        endwin();
        exit(0);
    }
}

int main() {
    // Установка обработки сигнала изменения размера окна
    signal(SIGWINCH, handle_resize);

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(100); // Немного увеличим тайм-аут для getch()

    generate_map();
    draw_map();

    int ch;
    while (1) {
        if (auto_play) {
            auto_move();
            draw_map();
            usleep(100000); // Задержка для автопрохождения
        }

        if ((ch = getch()) != ERR) {
            if (ch >= '0' && ch <= '9' && input_index < 4) {
                input_code[input_index++] = ch;
                input_code[input_index] = '\0';
                draw_map();
            } else if (ch == 10) { // Enter key
                check_code();
            } else if (input_index > 0 && ch == 127) { // Backspace key
                input_code[--input_index] = '\0';
                draw_map();
            } else {
                switch (ch) {
                    case 'w':
                        move_pacman(0, -1);
                        break;
                    case 's':
                        move_pacman(0, 1);
                        break;
                    case 'a':
                        move_pacman(-1, 0);
                        break;
                    case 'd':
                        move_pacman(1, 0);
                        break;
                    case 'e': // Переключение автопрохождения
                        auto_play = !auto_play;
                        break;
                    case 'k': // Переключение режима noclip
                        noclip = !noclip;
                        break;
                    case 'q': // Выход из игры
                        endwin();
                        printf("Game Over! Final Score: %d/%d\n", score, total_points);
                        return 0;
                }
                draw_map();
            }
        }
    }

    endwin();
    return 0;
}