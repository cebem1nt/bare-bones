#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define BOLD 1
#define REGULAR 0
#define NCOLORS 7
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

const char* COLORS[] = {
    "black", "red", "green", "yellow", "blue", "magenta", "cyan" 
};

struct args {
    char* color;
    int   is_shell,
          is_bold,
          is_bg;
};

void 
help(const char* program) 
{
    printf("\nUsage: %s [--shell|-s] [--bold|-b] [--background|-bg] [color]     \n"
           "       Get ansii color codes                                        \n"
           "                                                                    \n"
           "       --shell, -s:        Print shell escape codes                 \n"
           "       --bold, -b:         If color is provided, show bold color    \n"
           "                           instead of the regular                   \n"
           "       --background, -bg:  If color is provided, return background  \n"
           "                           instead of foreground color              \n"
           "                                                                    \n"
           "       color:              Optional color name: black, red, green,  \n"
           "                           yellow, blue, magenta, cyan              \n\n"
          , program);

    exit(EXIT_FAILURE);
}

struct args 
argparse(char** argv) 
{
    struct args out = {0};

    for (char** p = argv; *p != NULL; ++p) {
        char* arg = *p;

        if (IS(arg, "--help") || IS(arg, "-h"))
            help(argv[0]);
        else if (IS(arg, "--shell") || IS(arg, "-s"))
            out.is_shell = 1;
        else if (IS(arg, "--bold") || IS(arg, "-b"))
            out.is_bold = 1;
        else if (IS(arg, "--background") || IS(arg, "-bg"))
            out.is_bg = 1;
        else
            out.color = arg;
    }

    return out;
}

void 
print_escaped(char* s, int is_shell)
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
print_color(int is_shell, char* buf) 
{
    putchar(' ');
    printf("%s", buf);
    print_escaped(buf, is_shell);
    printf(RESET);
    putchar(' ');
}

int
main(int argc, char** argv) 
{
    struct args a = argparse(argv);    
    char prefix = a.is_shell ? '\e'
                             : '\x1B';

    if (a.color != NULL) {
        for (int i = 0; i < NCOLORS; i++) {
            const char* color = COLORS[i];
            
            if (!IS(a.color, color))
                continue;

            char buf[10];

            if (a.is_bg)
                ANSII_BG(prefix, a.is_bold, i, buf);
            else
                ANSII_FG(prefix, a.is_bold, i, buf);
            
            if (!isatty(STDOUT_FILENO)) {
                print_escaped(buf, a.is_shell);
            } else {
                printf("%s", buf);
                print_escaped(buf, a.is_shell);
                printf(RESET);            
            }

            putchar('\n');
            return 0;   
        }
    }

    printf("\n");
    for (int color = 0; color < NCOLORS; color++) {
        char buf[10];

        ANSII_FG(prefix, REGULAR, color, buf);
        print_color(a.is_shell, buf);

        ANSII_FG(prefix, BOLD, color, buf);
        print_color(a.is_shell, buf);

        ANSII_BG(prefix, REGULAR, color, buf);
        print_color(a.is_shell, buf);

        ANSII_BG(prefix, BOLD, color, buf);
        print_color(a.is_shell, buf);

        putchar('\n');
    }

    printf("\n Reset: ");
    print_escaped(RESET, a.is_shell);
    printf("\n\n");
}