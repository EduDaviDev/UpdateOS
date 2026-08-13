#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_INPUT 128
#define FS_COUNT 4
#define APP_TYPES 4

char disks_path[MAX_INPUT] = "./disks";
char apps_path[MAX_INPUT] = "./apps";
char app_type[MAX_INPUT] = "coded";
bool multi_fs_enabled = false;
bool fs_selected[FS_COUNT] = {true, false, false, false};
const char *fs_names[FS_COUNT] = {"FAT12", "FAT16", "FAT32", "ExFAT"};

bool multi_apps_enabled = false;
bool app_types_selected[APP_TYPES] = {true, false, false, false};
const char *app_type_names[APP_TYPES] = {"coded", "binaries", "objects", "multi"};

int runlevel = 1;
int highlight = 0;

void draw_box(int y, const char *title);
void draw_line(int y, const char *text);
int draw_main_menu(void);
int draw_apps_menu(void);
int draw_disks_menu(void);
int draw_fs_selector(void);
int draw_apps_selector(void);
void input_path(char *dest, const char *prompt);
void execute_option(int level, int choice);
int handle_key(int ch, int num_opts);

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int ch, num_opts;

    while (1) {
        clear();
        if (LINES < 15 || COLS < 40) {
            mvprintw(0, 0, "Terminal muito pequeno (min 15x40)");
            refresh();
            getch();
            continue;
        }

        switch (runlevel) {
            case 1:
                num_opts = draw_main_menu();
                refresh();
                ch = getch();
                if (handle_key(ch, num_opts)) continue;
                break;
            case 2:
                num_opts = draw_apps_menu();
                refresh();
                ch = getch();
                if (handle_key(ch, num_opts)) continue;
                break;
            case 3:
                num_opts = draw_disks_menu();
                refresh();
                ch = getch();
                if (handle_key(ch, num_opts)) continue;
                break;
            case 4:
                num_opts = draw_fs_selector();
                refresh();
                ch = getch();
                if (handle_key(ch, num_opts)) continue;
                break;
            case 5:
                num_opts = draw_apps_selector();
                refresh();
                ch = getch();
                if (handle_key(ch, num_opts)) continue;
                break;
            case 20:
                input_path(apps_path, "Enter apps base path: ");
                runlevel = 2;
                break;
            case 30:
                input_path(disks_path, "Enter disks folder path: ");
                runlevel = 3;
                break;
            case 40:
                {
                    char cmd[512];
                    if (multi_apps_enabled) {
                        char type_list[256] = "";
                        for (int i = 0; i < APP_TYPES; i++)
                            if (app_types_selected[i]) {
                                if (strlen(type_list) > 0) strcat(type_list, ",");
                                strcat(type_list, app_type_names[i]);
                            }
                        if (strlen(type_list) == 0) {
                            mvprintw(10, 0, "Nenhum tipo selecionado! Pressione qualquer tecla.");
                            refresh(); getch(); runlevel = 2; break;
                        }
                        snprintf(cmd, sizeof(cmd), "make apps APP_TYPE=multi APPS_BASE=%s", apps_path);
                    } else {
                        int idx = 0;
                        for (int i = 0; i < APP_TYPES; i++) if (app_types_selected[i]) { idx = i; break; }
                        snprintf(cmd, sizeof(cmd), "make apps APP_TYPE=%s APPS_BASE=%s", app_type_names[idx], apps_path);
                    }
                    endwin();
                    system(cmd);
                    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
                    runlevel = 2;
                }
                break;
            case 41:
    			{
    			    char cmd[512];
    			    if (multi_fs_enabled) {
    			        char fs_list[256] = "";
    			        for (int i = 0; i < FS_COUNT; i++) {
    			            if (fs_selected[i]) {
    			                if (strlen(fs_list) > 0) strcat(fs_list, " ");
    			                strcat(fs_list, fs_names[i]);
    			            }
    			        }
    			        if (strlen(fs_list) == 0) {
    			            mvprintw(10, 0, "Nenhum FS selecionado! Pressione qualquer tecla.");
    			            refresh();
    			            getch();
    			            runlevel = 3;
    			            break;
    			        }
    			        snprintf(cmd, sizeof(cmd),
    			                 "make disks DISK_PATH=%s MULTI_FS=true FS_LIST=\"%s\"",
    			                 disks_path, fs_list);
    			    } else {
    			        int idx = 0;
    			        for (int i = 0; i < FS_COUNT; i++) if (fs_selected[i]) { idx = i; break; }
    			        snprintf(cmd, sizeof(cmd),
    			                 "make disks DISK_PATH=%s MULTI_FS=false FS_LIST=\"%s\"",
    			                 disks_path, fs_names[idx]);
    			    }
    			    endwin();
    			    system(cmd);
    			    initscr();
    			    cbreak();
    			    noecho();
    			    keypad(stdscr, TRUE);
    			    curs_set(0);
    			    runlevel = 3;
    			}
    			break;
            default:
                runlevel = 1;
                break;
        }
    }

    endwin();
    return 0;
}

int handle_key(int ch, int num_opts) {
    if (ch == 27 || ch == 'q' || ch == 'Q') {
        endwin();
        exit(0);
    }
    if (ch >= '1' && ch <= '9') {
        int idx = ch - '1';
        if (idx < num_opts) {
            highlight = idx;
            execute_option(runlevel, highlight);
            return 1;
        }
    }
    if (ch == '0') {
        int idx = num_opts - 1;
        highlight = idx;
        execute_option(runlevel, highlight);
        return 1;
    }
    switch (ch) {
        case KEY_UP:
            highlight = (highlight - 1 + num_opts) % num_opts;
            return 1;
        case KEY_DOWN:
            highlight = (highlight + 1) % num_opts;
            return 1;
        case 10:
            execute_option(runlevel, highlight);
            return 1;
        default:
            return 0;
    }
}

void draw_box(int y, const char *title) {
    int width = COLS - 2;
    if (width < 10) width = 10;
    char line[width + 1];
    memset(line, '-', width);
    line[width] = '\0';
    mvprintw(y, 0, "+%s+", line);
    if (title && strlen(title) > 0) {
        int spaces = width - strlen(title);
        int left = spaces / 2;
        int right = spaces - left;
        mvprintw(y + 1, 0, "|%*s%s%*s|", left, "", title, right, "");
    } else {
        mvprintw(y + 1, 0, "|%*s|", width, "");
    }
    mvprintw(y + 2, 0, "+%s+", line);
}

void draw_line(int y, const char *text) {
    int width = COLS - 2;
    if (width < 10) width = 10;
    int len = strlen(text);
    int padding = width - len;
    if (padding < 0) padding = 0;
    mvprintw(y, 0, "| %s%*s |", text, padding, "");
}

int draw_main_menu(void) {
    int row = 0;
    draw_box(row++, "UpdateOS V1.0");
    row += 2;
    const char *opts[] = {"[1]  Build OS", "[2]  Build Apps", "[3]  Clean Build", "[4]  Make Disks", "[0]  Exit"};
    int num_opts = 5;
    for (int i = 0; i < num_opts; i++) {
        if (i == highlight) attron(A_REVERSE);
        draw_line(row++, opts[i]);
        if (i == highlight) attroff(A_REVERSE);
    }
    draw_box(row++, "");
    return num_opts;
}

int draw_apps_menu(void) {
    int row = 0;
    draw_box(row++, "UpdateOS V1.0 - Build Apps");
    row += 2;
    const char *opts[] = {"[1]  Set App Base Path", "[2]  Select App Types", "[3]  Build Apps!", "[0]  Back"};
    int num_opts = 4;
    for (int i = 0; i < num_opts; i++) {
        if (i == highlight) attron(A_REVERSE);
        draw_line(row++, opts[i]);
        if (i == highlight) attroff(A_REVERSE);
    }
    draw_box(row++, "");
    row++;
    mvprintw(row, 2, "Current base path: %s", apps_path);
    return num_opts;
}

int draw_disks_menu(void) {
    int row = 0;
    draw_box(row++, "UpdateOS V1.0 - Make Disks");
    row += 2;
    char multi_text[64];
    snprintf(multi_text, sizeof(multi_text), "[2]  Multi-FS %s", multi_fs_enabled ? "[X]" : "[ ]");
    const char *opts[] = {"[1]  Set Disks Folder Path", multi_text, "[3]  Select FSes", "[M]  Make Disks!", "[0]  Back"};
    int num_opts = 5;
    for (int i = 0; i < num_opts; i++) {
        if (i == highlight) attron(A_REVERSE);
        draw_line(row++, opts[i]);
        if (i == highlight) attroff(A_REVERSE);
    }
    draw_box(row++, "");
    row++;
    mvprintw(row, 2, "Current disks folder: %s", disks_path);
    return num_opts;
}

int draw_fs_selector(void) {
    int row = 0;
    draw_box(row++, multi_fs_enabled ? "Select FSes (Multi-FS)" : "Select FS (Single)");
    row += 2;
    int num_opts = FS_COUNT + (multi_fs_enabled ? 2 : 1);
    for (int i = 0; i < FS_COUNT; i++) {
        char line[64];
        if (multi_fs_enabled)
            snprintf(line, sizeof(line), "[%d]  %s %s", i+1, fs_names[i], fs_selected[i] ? "[X]" : "[ ]");
        else
            snprintf(line, sizeof(line), "[%d]  %s %s", i+1, fs_names[i], fs_selected[i] ? "(O)" : "( )");
        if (i == highlight) attron(A_REVERSE);
        draw_line(row++, line);
        if (i == highlight) attroff(A_REVERSE);
    }
    if (multi_fs_enabled) {
        int idx = FS_COUNT;
        if (idx == highlight) attron(A_REVERSE);
        draw_line(row++, "[5]  Toggle/Invert");
        if (idx == highlight) attroff(A_REVERSE);
    }
    int back_idx = multi_fs_enabled ? FS_COUNT + 1 : FS_COUNT;
    if (back_idx == highlight) attron(A_REVERSE);
    draw_line(row++, "[0]  Back");
    if (back_idx == highlight) attroff(A_REVERSE);
    draw_box(row++, "");
    row++;
    mvprintw(row, 2, multi_fs_enabled ? "Toggle: select multiple FSes" : "Radio: select only one FS");
    return num_opts;
}

int draw_apps_selector(void) {
    int row = 0;
    draw_box(row++, multi_apps_enabled ? "Select App Types (Multi)" : "Select App Type (Single)");
    row += 2;
    int num_opts = APP_TYPES + (multi_apps_enabled ? 2 : 1);
    for (int i = 0; i < APP_TYPES; i++) {
        char line[64];
        if (multi_apps_enabled)
            snprintf(line, sizeof(line), "[%d]  %s %s", i+1, app_type_names[i], app_types_selected[i] ? "[X]" : "[ ]");
        else
            snprintf(line, sizeof(line), "[%d]  %s %s", i+1, app_type_names[i], app_types_selected[i] ? "(O)" : "( )");
        if (i == highlight) attron(A_REVERSE);
        draw_line(row++, line);
        if (i == highlight) attroff(A_REVERSE);
    }
    if (multi_apps_enabled) {
        int idx = APP_TYPES;
        if (idx == highlight) attron(A_REVERSE);
        draw_line(row++, "[5]  Toggle/Invert");
        if (idx == highlight) attroff(A_REVERSE);
    }
    int back_idx = multi_apps_enabled ? APP_TYPES + 1 : APP_TYPES;
    if (back_idx == highlight) attron(A_REVERSE);
    draw_line(row++, "[0]  Back");
    if (back_idx == highlight) attroff(A_REVERSE);
    draw_box(row++, "");
    row++;
    mvprintw(row, 2, multi_apps_enabled ? "Toggle: select multiple types" : "Radio: select only one type");
    return num_opts;
}

void execute_option(int level, int choice) {
    switch (level) {
        case 1:
            switch (choice) {
                case 0: // Build OS
                    endwin();
                    system("make system");
                    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
                    break;
                case 1: // Build Apps
                    runlevel = 2;
                    break;
                case 2: // Clean Build
                    endwin();
                    system("make clean");
                    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
                    break;
                case 3: // Make Disks
                    runlevel = 3;
                    break;
                case 4: // Exit – NÃO COMPILA NADA!
                    endwin();
                    exit(0);
                    break;
            }
            break;
        case 2:
            switch (choice) {
                case 0: runlevel = 20; break;
                case 1: runlevel = 5; break;
                case 2: runlevel = 40; break;
                case 3: runlevel = 1; break;
            }
            break;
        case 3:
            switch (choice) {
                case 0: runlevel = 30; break;
                case 1:
                    multi_fs_enabled = !multi_fs_enabled;
                    if (!multi_fs_enabled) for (int i = 0; i < FS_COUNT; i++) fs_selected[i] = (i == 2);
                    break;
                case 2: runlevel = 4; break;
                case 3: runlevel = 41; break;
                case 4: runlevel = 1; break;
            }
            break;
        case 4:
            if (choice < FS_COUNT) {
                if (multi_fs_enabled) fs_selected[choice] = !fs_selected[choice];
                else for (int i = 0; i < FS_COUNT; i++) fs_selected[i] = (i == choice);
            } else if (multi_fs_enabled && choice == FS_COUNT) {
                int sel = 0;
                for (int i = 0; i < FS_COUNT; i++) if (fs_selected[i]) sel++;
                if (sel == FS_COUNT) for (int i = 0; i < FS_COUNT; i++) fs_selected[i] = false;
                else if (sel == 0) for (int i = 0; i < FS_COUNT; i++) fs_selected[i] = true;
                else for (int i = 0; i < FS_COUNT; i++) fs_selected[i] = !fs_selected[i];
            } else runlevel = 3;
            break;
        case 5:
            if (choice < APP_TYPES) {
                if (multi_apps_enabled) app_types_selected[choice] = !app_types_selected[choice];
                else for (int i = 0; i < APP_TYPES; i++) app_types_selected[i] = (i == choice);
            } else if (multi_apps_enabled && choice == APP_TYPES) {
                int sel = 0;
                for (int i = 0; i < APP_TYPES; i++) if (app_types_selected[i]) sel++;
                if (sel == APP_TYPES) for (int i = 0; i < APP_TYPES; i++) app_types_selected[i] = false;
                else if (sel == 0) for (int i = 0; i < APP_TYPES; i++) app_types_selected[i] = true;
                else for (int i = 0; i < APP_TYPES; i++) app_types_selected[i] = !app_types_selected[i];
            } else runlevel = 2;
            break;
    }
}

void input_path(char *dest, const char *prompt) {
    clear();
    mvprintw(0, 0, "%s", prompt);
    refresh();
    echo();
    curs_set(1);
    char input[MAX_INPUT] = {0};
    int pos = 0, ch;
    while (1) {
        ch = getch();
        if (ch == '\n' || ch == KEY_ENTER) break;
        else if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                pos--;
                input[pos] = '\0';
                mvprintw(1, 0, "%-*s", MAX_INPUT, input);
                move(1, pos);
                refresh();
            }
        }
        else if (ch >= 32 && ch <= 126) {
            if (pos < MAX_INPUT - 1) {
                input[pos++] = ch;
                input[pos] = '\0';
                mvprintw(1, 0, "%-*s", MAX_INPUT, input);
                move(1, pos);
                refresh();
            }
        }
    }
    strcpy(dest, input);
    noecho();
    curs_set(0);
}