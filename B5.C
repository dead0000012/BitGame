#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define WIDTH 40
#define HEIGHT 20

char map[HEIGHT][WIDTH + 1];
int pacman_x, pacman_y; // Позиция Пакмана
int score = 0;
int total_points = 0;
int noclip = 0; // Флаг для режима noclip
char input_code[5]; // Массив для ввода кода
int input_index = 0;
int auto_play_unlocked = 0; // Флаг разблокировки автопрохождения
int auto_play = 0; // Флаг автопрохождения
const char *correct_code = "8184";
const char *auto_play_code = "1111";
const char *noclip_code = "1234"; // Код для включения noclip

typedef struct Point {
    int x, y;
} Point;

typedef struct Node {
    Point point;
    int g, h; // g - стоимость пути от начальной точки, h - эвристическая стоимость
    struct Node *parent;
} Node;

typedef struct {
    Node* nodes[HEIGHT * WIDTH];
    int size;
} NodeList;

void generate_map() {
    srand(time(NULL));
    total_points = 0;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (y == 0 || y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {
                map[y][x] = '#'; // Границы карты
            } else {
                if (rand() % 5 == 0) {
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
    mvprintw(HEIGHT + 1, 0, "Noclip: %s", noclip ? "ON" : "OFF");
    mvprintw(HEIGHT + 2, 0, "Auto Play: %s", auto_play_unlocked ? (auto_play ? "ON" : "OFF") : "LOCKED");
    refresh();
}

void move_pacman(int dx, int dy) {
    int new_x = pacman_x + dx;
    int new_y = pacman_y + dy;

    if (noclip || map[new_y][new_x] != '#') { // Проверяем, чтобы Пакман не врезался в стену
        if (map[new_y][new_x] == '.') { // Если Пакман съел точку
            map[new_y][new_x] = ' ';
            score++;

            if (score == total_points) { // Все точки собраны
                clear();
                mvprintw(HEIGHT / 2, (WIDTH / 2) - 5, "Congratulations!");
                mvprintw((HEIGHT / 2) + 1, (WIDTH / 2) - 10, "Final Score: %d/%d", score, total_points);
                refresh();
                getch();
                endwin();
                exit(0);
            }
        }

        pacman_x = new_x;
        pacman_y = new_y;
    }
}

int heuristic(Point a, Point b) {
    return abs(a.x - b.x) + abs(a.y - b.y); // Манхэттенское расстояние
}

Node* find_node(NodeList* list, Point point) {
    for (int i = 0; i < list->size; i++) {
        if (list->nodes[i]->point.x == point.x && list->nodes[i]->point.y == point.y) {
            return list->nodes[i];
        }
    }
    return NULL;
}

void add_node(NodeList* list, Node* node) {
    list->nodes[list->size++] = node;
}

void remove_node(NodeList* list, Node* node) {
    for (int i = 0; i < list->size; i++) {
        if (list->nodes[i] == node) {
            for (int j = i; j < list->size - 1; j++) {
                list->nodes[j] = list->nodes[j + 1];
            }
            list->size--;
            return;
        }
    }
}

int node_comparator(const void* a, const void* b) {
    return (*(Node**)a)->g + (*(Node**)a)->h - (*(Node**)b)->g - (*(Node**)b)->h;
}

void reconstruct_path(Node* node) {
    while (node->parent != NULL) {
        move_pacman(node->point.x - node->parent->point.x, node->point.y - node->parent->point.y);
        node = node->parent;
        draw_map();
        usleep(100000);
    }
}

void auto_move() {
    Point directions[4] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    NodeList open_list = { .size = 0 };
    NodeList closed_list = { .size = 0 };

    Node* start_node = malloc(sizeof(Node));
    start_node->point.x = pacman_x;
    start_node->point.y = pacman_y;
    start_node->g = 0;
    start_node->h = 0;
    start_node->parent = NULL;
    add_node(&open_list, start_node);

    while (open_list.size > 0) {
        qsort(open_list.nodes, open_list.size, sizeof(Node*), node_comparator);
        Node* current = open_list.nodes[0];
        remove_node(&open_list, current);
        add_node(&closed_list, current);

        if (map[current->point.y][current->point.x] == '.') {
            reconstruct_path(current);
            return;
        }

        for (int i = 0; i < 4; i++) {
            Point neighbor_point = { .x = current->point.x + directions[i].x, .y = current->point.y + directions[i].y };
            if (map[neighbor_point.y][neighbor_point.x] == '#' || find_node(&closed_list, neighbor_point) != NULL) {
                continue;
            }

            int tentative_g = current->g + 1;
            Node* neighbor = find_node(&open_list, neighbor_point);

            if (neighbor == NULL) {
                neighbor = malloc(sizeof(Node));
                neighbor->point = neighbor_point;
                neighbor->g = tentative_g;
                neighbor->h = heuristic(neighbor_point, (Point){ .x = pacman_x, .y = pacman_y });
                neighbor->parent = current;
                add_node(&open_list, neighbor);
            } else if (tentative_g < neighbor->g) {
                neighbor->g = tentative_g;
                neighbor->parent = current;
            }
        }
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
    } else if (strcmp(input_code, auto_play_code) == 0) {
        auto_play_unlocked = 1;
        mvprintw(HEIGHT + 3, 0, "Auto Play unlocked! Use 'E' to toggle.");
        refresh();
    } else if (strcmp(input_code, noclip_code) == 0) {
        noclip = 1;
        mvprintw(HEIGHT + 3, 0, "Noclip mode activated!");
        refresh();
    }
    input_index = 0;
    input_code[0] = '\0';
}

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(100);

    generate_map();
    draw_map();

    int ch;
    while (1) {
        if (auto_play && auto_play_unlocked) {
            auto_move();
            draw_map();
            usleep(100000);
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
                        if (auto_play_unlocked) {
                            auto_play = !auto_play;
                        }
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