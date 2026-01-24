#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define TOTAL 7
#define RESET "\e[0m"

#define IS(arg, exp) (strcmp(arg, exp) == 0)
#define ANSII_FG(prefix, is_bold, c, buf) \
    do {                                  \
        buf[0] = prefix;                  \
        buf[1] = '[';                     \
        buf[2] = is_bold ? '1' : '0';     \
        buf[3] = ';';                     \
        buf[4] = '3';                     \
        buf[5] = '0' + c;                 \
        buf[6] = 'm';                     \
        buf[7] = '\0';                    \
    } while (0)

#define ANSII_BG(prefix, is_bold, c, buf) \
    do {                                  \
        buf[0] = prefix;                  \
        buf[1] = '[';                     \
        buf[2] = is_bold ? '1' : '0';     \
        buf[3] = ';';                     \
        buf[4] = '4';                     \
        buf[5] = '0' + c;                 \
        buf[6] = 'm';                     \
        buf[7] = '\0';                    \
    } while (0)

struct args {
    bool is_shell;
};

struct args 
argparse(char** argv) {
    struct args out;

    for (char** p = argv; *p != NULL; ++p) {
        char* arg = *p;

        if (IS(arg, "--shell") || IS(arg, "-s"))
            out.is_shell = true;
    }

    return out;
}

void 
print_escaped(char *s, bool is_shell)
{
    for (; *s != '\0'; ++s) {
        char c = *s;
        if (c == 0x1B) {
            fputs(is_shell ? "\\e" : "\\x1B", stdout);
        } else {
            putchar(c);
        }
    }
}

void 
print_color(bool is_shell, char* buf) 
{
    putchar(' ');
    printf("%s", buf);
    print_escaped(buf, is_shell);
    printf(RESET);
    putchar(' ');
}

void
print_colors(struct args* a) 
{
    char prefix = a->is_shell ? '\e'
                              : '\x1B';

    for (int c = 0; c < TOTAL; c++) {
        char buf[10];

        ANSII_FG(prefix, 0, c, buf);
        print_color(a->is_shell, buf);

        ANSII_FG(prefix, 1, c, buf);
        print_color(a->is_shell, buf);


        ANSII_BG(prefix, 0, c, buf);
        print_color(a->is_shell, buf);

        ANSII_BG(prefix, 1, c, buf);
        print_color(a->is_shell, buf);

        putchar('\n');
    }

    printf("\n Reset: ");
    print_escaped(RESET, a->is_shell);
    putchar('\n');
}

int
main(int argc, char** argv) 
{
    struct args a = argparse(argv);
    print_colors(&a);
}