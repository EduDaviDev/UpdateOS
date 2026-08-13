#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char **argv) {
    if (argc != 2) return 0;
    FILE *f = fopen(argv[1], "r");
    if (!f) return 0;
    char line[256];
    unsigned long stack = 0, entry = 0;
    int stack_set = 0, entry_set = 0;
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p && isspace(*p)) p++;
        if (*p == '#' || *p == '\0') continue;
        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = p;
        char *val = eq+1;
        while (isspace(*key)) key++;
        char *kend = key + strlen(key) - 1;
        while (kend > key && isspace(*kend)) *kend-- = '\0';
        while (*val && isspace(*val)) val++;
        char *vend = val + strlen(val) - 1;
        while (vend > val && isspace(*vend)) *vend-- = '\0';
        if (strcmp(key, "STACK_SIZE") == 0) {
            stack = strtoul(val, NULL, 0);
            stack_set = 1;
        } else if (strcmp(key, "ENTRY_OFFSET") == 0) {
            entry = strtoul(val, NULL, 0);
            entry_set = 1;
        }
    }
    fclose(f);
    if (stack_set) printf("STACK_SIZE=%lu ", stack);
    if (entry_set) printf("ENTRY_OFFSET=%lu ", entry);
    printf("\n");
    return 0;
}