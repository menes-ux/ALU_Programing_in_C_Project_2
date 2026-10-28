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