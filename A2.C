#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // Для функции usleep

#define WIDTH 40
#define HEIGHT 20

char map[HEIGHT][WIDTH + 1];
int pacman_x, pacman_y; // Позиция Пакмана
int score = 0;
int total_points = 0;
int auto_play = 0; // Флаг для автопрохождения
int noclip = 0; // Флаг для режима noclip

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
    for (int i = 0; i < 4; i++) {
        int new_x = pacman_x + directions[i][0];
        int new_y = pacman_y + directions[i][1];
        if (map[new_y][new_x] != '#' && map[new_y][new_x] == '.') {
            move_pacman(directions[i][0], directions[i][1]);
            return;
        }
    }
    for (int i = 0; i < 4; i++) { // Если нет точек, просто двигаться
        int new_x = pacman_x + directions[i][0];
        int new_y = pacman_y + directions[i][1];
        if (map[new_y][new_x] != '#') {
            move_pacman(directions[i][0], directions[i][1]);
            return;
        }
    }
}

int main() {
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

    endwin();
    return 0;
}