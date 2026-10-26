#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char name[80];
    char course[80];
    float grade;
} Student;

/* I return 1 for valid input, -1 for invalid input, and 0 when input ends. */
static int read_line(const char *prompt, char *buffer, size_t buffer_size)
{
    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        return 0;
    }

    char *newline = strchr(buffer, '\n');
    if (newline != NULL) {
        *newline = '\0';
    } else {
        /* I discard the rest of a long line before asking again. */
        int character = getchar();
        if (character != '\n' && character != EOF) {
            while ((character = getchar()) != '\n' && character != EOF) {
            }
            printf("Input is too long. Maximum length is %zu characters.\n",
                   buffer_size - 1);
            return -1;
        }
    }

    char *start = buffer;
    while (isspace((unsigned char)*start)) {
        ++start;
    }

    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        --end;
    }
    *end = '\0';

    if (start == end) {
        puts("Input cannot be empty.");
        return -1;
    }

    memmove(buffer, start, (size_t)(end - start) + 1);
    return 1;
}

static int read_integer(const char *prompt, int *result)
{
    char input[128];
    int input_status;

    while ((input_status = read_line(prompt, input, sizeof(input))) != 0) {
        if (input_status == -1) {
            continue;
        }

        char *end;
        errno = 0;
        long value = strtol(input, &end, 10);
        if (errno == ERANGE || end == input || *end != '\0' ||
            value < INT_MIN || value > INT_MAX) {
            puts("Enter a valid whole number.");
            continue;
        }

        *result = (int)value;
        return 1;
    }

    return 0;
}

static int read_grade(float *grade)
{
    char input[128];
    int input_status;

    while ((input_status = read_line("Grade (0-100): ", input,
                                     sizeof(input))) != 0) {
        if (input_status == -1) {
            continue;
        }

        char *end;
        errno = 0;
        double value = strtod(input, &end);
        if (errno == ERANGE || end == input || *end != '\0' ||
            !(value >= 0.0 && value <= 100.0)) {
            puts("Enter a numerical grade between 0 and 100.");
            continue;
        }

        *grade = (float)value;
        return 1;
    }

    return 0;
}

static int student_id_exists(const Student *students, size_t record_count, int id)
{
    const Student *current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        if (current->id == id) {
            return 1;
        }
    }
    return 0;
}

static void display_students(const Student *students, size_t record_count)
{
    if (record_count == 0) {
        puts("No student records to display.");
        return;
    }

    int name_width = 9;
    int course_width = 6;

    /* I find the column widths so long names are still shown in full. */
    const Student *current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        int name_length = (int)strlen(current->name);
        int course_length = (int)strlen(current->course);
        if (name_length > name_width) {
            name_width = name_length;
        }
        if (course_length > course_width) {
            course_width = course_length;
        }
    }

    printf("\n%-10s | %-*s | %-*s | %6s\n", "ID", name_width, "Full name",
           course_width, "Course", "Grade");

    current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        printf("%-10d | %-*s | %-*s | %6.2f\n", current->id,
               name_width, current->name, course_width, current->course,
               current->grade);
    }
    printf("Total records: %zu\n", record_count);
}

/* I return 1 after adding, 0 when input ends, and -1 if allocation fails. */
static int add_student(Student **students, size_t *record_count, size_t *capacity)
{
    Student new_student;

    while (1) {
        if (!read_integer("Student ID: ", &new_student.id)) {
            return 0;
        }
        if (new_student.id <= 0) {
            puts("Student ID must be a positive whole number.");
        } else if (student_id_exists(*students, *record_count, new_student.id)) {
            puts("This student ID already exists. Enter a different ID.");
        } else {
            break;
        }
    }

    int input_status;
    do {
        input_status = read_line("Full name: ", new_student.name,
                                 sizeof(new_student.name));
    } while (input_status == -1);
    if (input_status == 0) {
        return 0;
    }

    do {
        input_status = read_line("Course: ", new_student.course,
                                 sizeof(new_student.course));
    } while (input_status == -1);
    if (input_status == 0 || !read_grade(&new_student.grade)) {
        return 0;
    }

    if (*record_count == *capacity) {
        if (*capacity > SIZE_MAX / 2 / sizeof(Student)) {
            fputs("Cannot allocate space for more students.\n", stderr);
            return -1;
        }

        size_t new_capacity = *capacity == 0 ? 1 : *capacity * 2;
        Student *resized_students = realloc(*students,
                                            new_capacity * sizeof(Student));
        if (resized_students == NULL) {
            fputs("Could not grow the student array. Existing records are kept.\n",
                  stderr);
            return -1;
        }

        *students = resized_students;
        *capacity = new_capacity;
    }

    (*students)[*record_count] = new_student;
    ++(*record_count);
    printf("Student added successfully. Records: %zu. Capacity: %zu.\n",
           *record_count, *capacity);
    return 1;
}

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

    int running = 1;
    while (running) {
        puts("\nTemporary interface: 1 = Display, 2 = Add, 0 = Exit");
        int choice;
        if (!read_integer("Choice: ", &choice)) {
            puts("\nInput ended. Closing the program.");
            break;
        }

        switch (choice) {
            case 1:
                display_students(students, record_count);
                break;
            case 2:
                if (add_student(&students, &record_count, &capacity) == 0) {
                    puts("\nInput ended. Incomplete record was not added.");
                    running = 0;
                }
                break;
            case 0:
                running = 0;
                break;
            default:
                puts("Choose 1, 2, or 0.");
                break;
        }
    }

    free(students);
    return EXIT_SUCCESS;
}
