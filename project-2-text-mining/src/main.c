#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct wordStat {
    char name[40];
    size_t count;
} WordStat;

typedef struct {
    char *text;
    WordStat *words;
    size_t total;
    size_t distinct;
    char source[256];
} TextAnalysis;

static int is_letter(char character)
{
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
}

/* Read a bounded answer, retrying empty or overlong lines. */
static int read_line(const char *prompt, char *buffer, size_t size)
{
    const char *whitespace = " \t\r\n\v\f";
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(buffer, (int)size, stdin) == NULL) {
            return 0;
        }
        char *newline = strchr(buffer, '\n');
        if (newline != NULL) {
            *newline = '\0';
        } else {
            int character = getchar();
            if (character == '\r') {
                character = getchar();
            }
            if (character != '\n' && character != EOF) {
                while ((character = getchar()) != '\n' && character != EOF) {
                }
                printf("Input is too long. Maximum: %zu characters.\n", size - 1);
                continue;
            }
        }
        char *start = buffer + strspn(buffer, whitespace);
        char *end = start + strlen(start);
        while (end > start && strchr(whitespace, end[-1]) != NULL) {
            --end;
        }
        *end = '\0';
        if (start == end) {
            puts("Input cannot be empty.");
            continue;
        }
        memmove(buffer, start, (size_t)(end - start) + 1);
        return 1;
    }
}

static int read_choice(const char *prompt, int maximum, int *choice)
{
    char input[80];
    while (read_line(prompt, input, sizeof(input))) {
        char *end;
        long value = strtol(input, &end, 10);
        /* Check the small menu range before converting to int. Even strtol
           overflow returns a value outside this range, so errno is not needed. */
        if (end != input && *end == '\0' && value >= 1 && value <= maximum) {
            *choice = (int)value;
            return 1;
        }
        printf("Choose a whole number from 1 to %d.\n", maximum);
    }
    return 0;
}