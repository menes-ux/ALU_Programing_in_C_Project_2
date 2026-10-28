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

/* Replace the current analysis only after the new statistics are complete.
   On success, this function takes ownership of text; on failure, the caller
   must free text. Words longer than 39 letters reject the whole new text. */
static int analyse_text(TextAnalysis *analysis, char *text, const char *source)
{
    WordStat *words = NULL;
    size_t total = 0;
    size_t distinct = 0;
    const char *current = text;

    while (*current != '\0') {
        while (*current != '\0' && !is_letter(*current)) {
            ++current;
        }
        if (*current == '\0') {
            break;
        }
        char word[40];
        size_t length = 0;
        while (is_letter(*current)) {
            if (length < sizeof(word) - 1) {
                word[length] = (*current >= 'A' && *current <= 'Z')
                                   ? (char)(*current - 'A' + 'a') : *current;
            }
            ++length;
            ++current;
        }
        if (length >= sizeof(word)) {
            puts("Text rejected: a word exceeds the 39-letter limit.");
            free(words);
            return 0;
        }
        word[length] = '\0';

        size_t index = 0;
        while (index < distinct && strcmp(words[index].name, word) != 0) {
            ++index;
        }
        if (index == distinct) {
            if (distinct >= (size_t)-1 / sizeof(WordStat)) {
                fputs("Too many distinct words to store.\n", stderr);
                free(words);
                return 0;
            }
            WordStat *resized = realloc(words, (distinct + 1) * sizeof(WordStat));
            if (resized == NULL) {
                fputs("Could not allocate memory for word statistics.\n", stderr);
                free(words);
                return 0;
            }
            words = resized;
            strcpy(words[distinct].name, word);
            words[distinct].count = 1;
            ++distinct;
        } else {
            ++words[index].count;
        }
        ++total;
    }

    free(analysis->text);
    free(analysis->words);
    analysis->text = text;
    analysis->words = words;
    analysis->total = total;
    analysis->distinct = distinct;
    strcpy(analysis->source, source);
    return 1;
}

static int load_sample(TextAnalysis *analysis)
{
    const char *paths[] = {
        "project-2-text-mining/data/sample_text.txt",
        "data/sample_text.txt",
        "../data/sample_text.txt"
    };
    FILE *file = NULL;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        file = fopen(paths[i], "r");
        if (file != NULL) {
            break;
        }
    }
    if (file == NULL) {
        fputs("Could not open data/sample_text.txt. Run from the repository root,\n"
              "project-2-text-mining, or its src directory.\n", stderr);
        return 0;
    }
    char *text = read_text(file, 0);
    fclose(file);
    if (text == NULL) {
        return 0;
    }
    if (!analyse_text(analysis, text,
                      "Nineteen Eighty-Four (1984)\nAuthor: George Orwell\n"
                      "Publication year: 1949\nSection: Part One, Chapter 1\nPDF page: 5")) {
        free(text);
        return 0;
    }
    if (analysis->total < 150) {
        fputs("The published sample must contain at least 150 words.\n", stderr);
        return 0;
    }
    return 1;
}