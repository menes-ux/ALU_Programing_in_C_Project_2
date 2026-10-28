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

/* File input ends at EOF. Pasted input ends at an empty line or EOF. */
static char *read_text(FILE *stream, int stop_at_empty_line)
{
    size_t capacity = 256;
    size_t length = 0;
    char *text = malloc(capacity);
    if (text == NULL) {
        fputs("Could not allocate memory for the text.\n", stderr);
        return NULL;
    }

    int character;
    while ((character = fgetc(stream)) != EOF) {
        if (character == '\r') {
            continue;
        }
        if (stop_at_empty_line && character == '\n' &&
            (length == 0 || text[length - 1] == '\n')) {
            break;
        }
        if (character == '\0') {
            fputs("Text cannot contain a null character.\n", stderr);
            free(text);
            return NULL;
        }
        if (length == capacity - 1) {
            if (capacity > (size_t)-1 / 2) {
                fputs("The text is too large to store.\n", stderr);
                free(text);
                return NULL;
            }
            char *resized = realloc(text, capacity * 2);
            if (resized == NULL) {
                fputs("Could not grow the text buffer.\n", stderr);
                free(text);
                return NULL;
            }
            text = resized;
            capacity *= 2;
        }
        text[length++] = (char)character;
    }
    if (ferror(stream)) {
        fputs("An error occurred while reading the text.\n", stderr);
        free(text);
        return NULL;
    }
    text[length] = '\0';
    return text;
}