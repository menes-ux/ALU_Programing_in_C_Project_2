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

/* All menu operations return 1 to continue, or 0 when input ends. */
static int count_words(TextAnalysis *analysis)
{
    printf("Total words: %zu\nDistinct words: %zu\n",
           analysis->total, analysis->distinct);
    return 1;
}

static int longest_word(TextAnalysis *analysis)
{
    if (analysis->distinct == 0) {
        puts("No words available for a longest word.");
        return 1;
    }
    size_t longest = 0;
    for (size_t i = 0; i < analysis->distinct; ++i) {
        size_t length = strlen(analysis->words[i].name);
        if (length > longest) {
            longest = length;
        }
    }
    printf("Longest word(s), %zu letters:\n", longest);
    for (size_t i = 0; i < analysis->distinct; ++i) {
        if (strlen(analysis->words[i].name) == longest) {
            puts(analysis->words[i].name);
        }
    }
    return 1;
}

static int most_frequent(TextAnalysis *analysis)
{
    if (analysis->distinct == 0) {
        puts("No words available for a most frequent word.");
        return 1;
    }
    size_t highest = 0;
    for (size_t i = 0; i < analysis->distinct; ++i) {
        if (analysis->words[i].count > highest) {
            highest = analysis->words[i].count;
        }
    }
    printf("Most frequent word(s), %zu occurrence(s) each:\n", highest);
    for (size_t i = 0; i < analysis->distinct; ++i) {
        if (analysis->words[i].count == highest) {
            puts(analysis->words[i].name);
        }
    }
    return 1;
}

static int display_word_statistics(TextAnalysis *analysis)
{
    if (analysis->distinct == 0) {
        puts("No words found in the current text.");
        return 1;
    }
    printf("\n%-39s | %s\n", "Word", "Frequency");
    for (size_t i = 0; i < analysis->distinct; ++i) {
        printf("%-39s | %zu\n", analysis->words[i].name, analysis->words[i].count);
    }
    return 1;
}

static int compare_alphabetically(const void *left, const void *right)
{
    const WordStat *a = left;
    const WordStat *b = right;
    return strcmp(a->name, b->name);
}

static int compare_frequency(const void *left, const void *right)
{
    const WordStat *a = left;
    const WordStat *b = right;
    if (a->count == b->count) {
        return strcmp(a->name, b->name);
    }
    return (a->count < b->count) - (a->count > b->count);
}

static int sort_word_statistics(TextAnalysis *analysis)
{
    if (analysis->distinct == 0) {
        puts("No word statistics to sort.");
        return 1;
    }
    int choice;
    if (!read_choice("Sort by 1 = Alphabetical, 2 = Frequency (highest first): ",
                     2, &choice)) {
        return 0;
    }
    int (*compare)(const void *, const void *) =
        choice == 1 ? compare_alphabetically : compare_frequency;
    qsort(analysis->words, analysis->distinct, sizeof(WordStat), compare);
    return display_word_statistics(analysis);
}

static int search_word(TextAnalysis *analysis)
{
    char word[40];
    if (!read_line("Word to search for (letters only): ", word, sizeof(word))) {
        return 0;
    }
    for (char *current = word; *current != '\0'; ++current) {
        if (!is_letter(*current)) {
            puts("Enter one word containing only letters A-Z or a-z.");
            return 1;
        }
        if (*current >= 'A' && *current <= 'Z') {
            *current = (char)(*current - 'A' + 'a');
        }
    }
    for (size_t i = 0; i < analysis->distinct; ++i) {
        if (strcmp(analysis->words[i].name, word) == 0) {
            printf("%s: %zu occurrence(s)\n", word, analysis->words[i].count);
            return 1;
        }
    }
    printf("%s: 0 occurrences\n", word);
    return 1;
}

static int lexical_diversity(TextAnalysis *analysis)
{
    puts("\nCustom Analysis: Lexical Diversity Score");
    puts("Formula: distinct words / total words.");
    puts("Higher scores indicate more varied vocabulary in this passage;");
    puts("repeating words reduces the score. Text length also affects the score.");
    if (analysis->total == 0) {
        puts("Lexical Diversity Score: N/A (no words).");
    } else {
        double score = (double)analysis->distinct / analysis->total;
        printf("Lexical Diversity Score: %.4f (%.2f%%)\n", score, score * 100.0);
    }
    return 1;
}

static int generate_text_summary(TextAnalysis *analysis)
{
    puts("\nText Summary");
    printf("Source: %s\n", analysis->source);
    count_words(analysis);
    if (analysis->total == 0) {
        puts("Average word length: N/A");
    } else {
        double letters = 0.0;
        for (size_t i = 0; i < analysis->distinct; ++i) {
            letters += (double)strlen(analysis->words[i].name) *
                       analysis->words[i].count;
        }
        printf("Average word length: %.2f letters\n", letters / analysis->total);
    }
    longest_word(analysis);
    most_frequent(analysis);
    return lexical_diversity(analysis);
}

static int inspect_memory(TextAnalysis *analysis)
{
    puts("\nMemory Inspection");
    printf("Text buffer: %p (start of the stored paragraph)\n",
           (void *)analysis->text);
    printf("Word statistics array: %p (start of the word records)\n",
           (void *)analysis->words);
    if (analysis->distinct == 0) {
        puts("Selected word-stat entry: N/A (no word records).");
    } else {
        WordStat *selected = analysis->words + analysis->distinct / 2;
        printf("Selected word-stat entry: %p (record for '%s')\n",
               (void *)selected, selected->name);
    }
    return 1;
}

static int enter_text(TextAnalysis *analysis)
{
    char source[256];
    if (!read_line("Source title and author/publication: ", source, sizeof(source))) {
        return 0;
    }
    puts("Enter or paste your paragraph. Finish with an empty line:");
    fflush(stdout);
    char *text = read_text(stdin, 1);
    if (text == NULL) {
        return 0;
    }
    if (*text == '\0') {
        puts("Empty input. The current paragraph is kept.");
        free(text);
        return 1;
    }
    if (!analyse_text(analysis, text, source)) {
        free(text);
        puts("The current paragraph and statistics are kept.");
        return 1;
    }
    printf("\nSource: %s\nNew text loaded: %zu words.\n",
           analysis->source, analysis->total);
    return 1;
}

int main(void)
{
    char student_id[80];
    if (!read_line("Your student ID: ", student_id, sizeof(student_id))) {
        return EXIT_SUCCESS;
    }
    TextAnalysis analysis = {0};
    if (!load_sample(&analysis)) {
        free(analysis.text);
        free(analysis.words);
        return EXIT_FAILURE;
    }

    printf("\nStudent ID: %s\n", student_id);
    puts("Project: Dynamic Text Mining and Linguistic Pattern Analyzer");
    printf("Source: %s\n", analysis.source);
    puts("Custom Analysis: Lexical Diversity Score");
    puts("\nWord rules: words are sequences of English letters A-Z or a-z.");
    puts("Case is ignored. Punctuation, apostrophes, hyphens, numbers, and");
    puts("whitespace separate words: 'don't' becomes 'don' and 't';");
    puts("'thirty-nine' becomes 'thirty' and 'nine'. Other characters separate words.");
    puts("Words longer than 39 letters reject the entire new paragraph.");
    printf("\nLoaded paragraph (%zu words):\n%s\n", analysis.total, analysis.text);

    int (*operations[])(TextAnalysis *) = {
        count_words, longest_word, most_frequent, display_word_statistics,
        sort_word_statistics, search_word, generate_text_summary,
        lexical_diversity, inspect_memory, enter_text
    };
    while (1) {
        puts("\n1. Count words\n2. Find longest word\n3. Find most frequent word\n"
             "4. Display word statistics\n5. Sort word statistics\n6. Search word\n"
             "7. Display text summary\n8. Lexical Diversity Score\n"
             "9. Inspect memory addresses\n10. Enter a new paragraph\n11. Exit");
        int choice;
        if (!read_choice("Choice: ", 11, &choice) || choice == 11) {
            break;
        }
        if (!operations[choice - 1](&analysis)) {
            break;
        }
    }
    free(analysis.text);
    free(analysis.words);
    return EXIT_SUCCESS;
}