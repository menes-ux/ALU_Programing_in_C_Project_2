#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[80];
    char course[80];
    float grade;
} Student;

int main(void)
{
    /* I use fictional students for the initial dataset. */
    const Student initial_students[] = {
        {1001, "Amara Niyonsaba", "Programming in C", 84.0f},
        {1002, "Daniel Okello", "Mathematics", 72.5f},
        {1003, "Grace Mukamana", "Programming in C", 91.0f},
        {1004, "Samuel Mensah", "Statistics", 48.0f},
        {1005, "Aisha Kamara", "Mathematics", 66.0f},
        {1006, "Eric Habimana", "Programming in C", 55.5f},
        {1007, "Lydia Achieng", "Statistics", 78.0f},
        {1008, "Noah Dlamini", "Mathematics", 39.0f},
        {1009, "Fatima Diallo", "Programming in C", 87.5f},
        {1010, "Peter Mutesa", "Statistics", 60.0f},
        {1011, "Zara Ncube", "Mathematics", 95.0f},
        {1012, "Isaac Adeyemi", "Programming in C", 50.0f}
    };

    const size_t initial_count =
        sizeof(initial_students) / sizeof(initial_students[0]);
    size_t record_count = 0;
    size_t capacity = initial_count;

    Student *students = malloc(capacity * sizeof(Student));
    if (students == NULL) {
        fprintf(stderr, "Could not allocate memory for student records.\n");
        return EXIT_FAILURE;
    }

    /* I copy each record into the allocated array. */
    for (size_t i = 0; i < initial_count; ++i) {
        students[i] = initial_students[i];
        ++record_count;
    }

    printf("Initialization complete: %zu student records loaded.\n",
           record_count);

    free(students);
    return EXIT_SUCCESS;
}
