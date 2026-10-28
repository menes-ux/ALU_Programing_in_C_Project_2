#include <errno.h> /*serves as a flag for error reporting*/
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASS_GRADE 50.0f

typedef struct Student {
    int id;
    char name[80];
    char course[80];
    float grade;
} Student;

/* Keep the dynamic array and its record count together for menu operations. */
typedef struct {
    Student *students;
    size_t count;
} StudentList;

/* Retry invalid input. Return 1 for a nonempty line, or 0 when input ends. */
static int read_line(const char *prompt, char *buffer, size_t buffer_size)
{
    const char *whitespace = " \t\r\n\v\f";
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
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
                /* Discard the remainder so it cannot become the next answer. */
                while ((character = getchar()) != '\n' && character != EOF) {
                }
                printf("Input is too long. Maximum length is %zu characters.\n",
                       buffer_size - 1);
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

static int read_integer(const char *prompt, int *result)
{
    char input[128];
    while (read_line(prompt, input, sizeof(input))) {
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
    while (read_line("Grade (0-100): ", input, sizeof(input))) {
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

static void print_student_table(const Student *students, size_t record_count)
{
    if (record_count == 0) {
        puts("No student records to display.");
        return;
    }

    printf("\n%-10s | %-25s | %-25s | %6s\n",
           "ID", "Full name", "Course", "Grade");

    /* Traverse records using a pointer. Strings are displayed in full. */
    const Student *current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        printf("%-10d | %-25s | %-25s | %6.2f\n", current->id,
               current->name, current->course, current->grade);
    }
    printf("Total records: %zu\n", record_count);
}

static int display_students(StudentList *records)
{
    print_student_table(records->students, records->count);
    return 1;
}

/* Menu functions return 0 when input ends, or 1 to continue the menu. */
static int add_student(StudentList *records)
{
    Student new_student;

    while (1) {
        if (!read_integer("Student ID: ", &new_student.id)) {
            return 0;
        }
        if (new_student.id <= 0) {
            puts("Student ID must be a positive whole number.");
        } else if (student_id_exists(records->students, records->count, new_student.id)) {
            puts("This student ID already exists. Enter a different ID.");
        } else {
            break;
        }
    }

    if (!read_line("Full name: ", new_student.name, sizeof(new_student.name)) ||
        !read_line("Course: ", new_student.course, sizeof(new_student.course)) ||
        !read_grade(&new_student.grade)) {
        return 0;
    }

    /* (size_t)-1 is the largest size_t; prevent allocation-size overflow. */
    if (records->count >= (size_t)-1 / sizeof(Student)) {
        fputs("Cannot allocate space for another student.\n", stderr);
        return 1;
    }
    Student *resized_students = realloc(records->students,
                                        (records->count + 1) * sizeof(Student));
    if (resized_students == NULL) {
        fputs("Could not add student. Existing records are kept.\n", stderr);
        return 1;
    }
    records->students = resized_students;

    records->students[records->count] = new_student;
    ++records->count;
    printf("Student added successfully. Total records: %zu\n", records->count);
    return 1;
}

static double average_grade(const Student *students, size_t record_count)
{
    if (record_count == 0) {
        return 0.0;
    }

    double total = 0.0;
    const Student *current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        total += current->grade;
    }
    return total / record_count;
}

static int calculate_average(StudentList *records)
{
    if (records->count == 0) {
        puts("No student records available for an average.");
        return 1;
    }
    printf("Average grade: %.2f\n", average_grade(records->students, records->count));
    return 1;
}

static int find_top_student(StudentList *records)
{
    const Student *students = records->students;
    size_t record_count = records->count;
    if (record_count == 0) {
        puts("No student records available for a top performer.");
        return 1;
    }

    float highest = students->grade;
    const Student *current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        if (current->grade > highest) {
            highest = current->grade;
        }
    }

    puts("Top-performing students (including ties):");
    current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        if (current->grade == highest) {
            print_student_table(current, 1);
        }
    }
    return 1;
}

static size_t risk_category(float grade)
{
    if (grade >= 80.0f) {
        return 3;
    }
    if (grade >= 60.0f) {
        return 2;
    }
    if (grade >= PASS_GRADE) {
        return 1;
    }
    return 0;
}

static int custom_analysis(StudentList *records)
{
    const Student *students = records->students;
    size_t record_count = records->count;
    const char *labels[] = {"At Risk", "Needs Improvement", "Satisfactory", "Excellent"};
    size_t counts[4] = {0};

    puts("\nCustom Analysis: Academic Risk Classification");
    puts("Purpose: show grade distribution and identify students needing support.");
    puts("At Risk: 0 <= grade < 50; Needs Improvement: 50 <= grade < 60;");
    puts("Satisfactory: 60 <= grade < 80; Excellent: 80 <= grade <= 100.");
    puts("These thresholds are project rules, not official university grades.");
    puts("Category share (%) = category count / total records * 100.");
    puts("Students below 60 are flagged for academic support.");

    if (record_count == 0) {
        puts("No student records available for risk analysis.");
        return 1;
    }

    const Student *current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        ++counts[risk_category(current->grade)];
    }
    for (size_t category = 0; category < 4; ++category) {
        printf("%-18s: %zu (%.2f%%)\n", labels[category], counts[category],
               counts[category] * 100.0 / record_count);
    }

    size_t support_count = counts[0] + counts[1];
    printf("Students needing academic support: %zu\n", support_count);
    current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        size_t category = risk_category(current->grade);
        if (category < 2) {
            printf("Support: ID %d | %s | Grade %.2f | %s\n",
                   current->id, current->name, current->grade, labels[category]);
        }
    }
    return 1;
}

static int generate_summary_report(StudentList *records)
{
    const Student *students = records->students;
    size_t record_count = records->count;
    puts("\nAcademic Summary Report");
    printf("Total student records: %zu\n", record_count);

    size_t passing = 0;
    if (record_count == 0) {
        puts("Average grade: N/A\nHighest grade: N/A\nLowest grade: N/A");
    } else {
        float highest = students->grade;
        float lowest = students->grade;
        const Student *current = students;
        for (size_t i = 0; i < record_count; ++i, ++current) {
            if (current->grade > highest) {
                highest = current->grade;
            }
            if (current->grade < lowest) {
                lowest = current->grade;
            }
            if (current->grade >= PASS_GRADE) {
                ++passing;
            }
        }
        printf("Average grade: %.2f\n", average_grade(students, record_count));
        printf("Highest grade: %.2f\nLowest grade: %.2f\n", highest, lowest);
    }

    printf("Students meeting pass threshold (%.0f): %zu\n", PASS_GRADE, passing);
    printf("Students below pass threshold (%.0f): %zu\n", PASS_GRADE,
           record_count - passing);
    custom_analysis(records);
    return 1;
}

static int compare_student_ids(const void *left, const void *right)
{
    const Student *first = left;
    const Student *second = right;
    return (first->id > second->id) - (first->id < second->id);
}

static int compare_student_names(const void *left, const void *right)
{
    const Student *first = left;
    const Student *second = right;
    return strcmp(first->name, second->name);
}

static int compare_student_grades(const void *left, const void *right)
{
    const Student *first = left;
    const Student *second = right;
    return (first->grade < second->grade) - (first->grade > second->grade);
}

static int sort_students(StudentList *records)
{
    Student *students = records->students;
    size_t record_count = records->count;
    if (record_count == 0) {
        puts("No student records to sort.");
        return 1;
    }

    int choice;
    if (!read_integer("Sort by 1 = Grade, 2 = Name, 3 = ID: ", &choice)) {
        return 0;
    }

    int (*compare)(const void *, const void *);
    if (choice == 1) {
        compare = compare_student_grades;
        puts("Sorting by grade, highest first.");
    } else if (choice == 2) {
        compare = compare_student_names;
        puts("Sorting by name using case-sensitive alphabetical order.");
    } else if (choice == 3) {
        compare = compare_student_ids;
        puts("Sorting by student ID, lowest first.");
    } else {
        puts("Choose 1, 2, or 3 for the sort type.");
        return 1;
    }

    /* I pass the selected comparison function to qsort. */
    qsort(students, record_count, sizeof(Student), compare);
    print_student_table(students, record_count);
    return 1;
}

static int search_student(StudentList *records)
{
    const Student *students = records->students;
    size_t record_count = records->count;
    if (record_count == 0) {
        puts("No student records to search.");
        return 1;
    }

    int choice;
    if (!read_integer("Search by 1 = ID, 2 = Full name: ", &choice)) {
        return 0;
    }

    int id = 0;
    char name[80];
    if (choice == 1) {
        if (!read_integer("Student ID: ", &id)) {
            return 0;
        }
    } else if (choice == 2) {
        puts("Name matching is exact and case-sensitive.");
        if (!read_line("Full name: ", name, sizeof(name))) {
            return 0;
        }
    } else {
        puts("Choose 1 or 2 for the search type.");
        return 1;
    }

    size_t matches = 0;
    const Student *current = students;
    for (size_t i = 0; i < record_count; ++i, ++current) {
        if ((choice == 1 && current->id == id) ||
            (choice == 2 && strcmp(current->name, name) == 0)) {
            print_student_table(current, 1);
            ++matches;
        }
    }
    if (matches == 0) {
        puts("No matching student found.");
    } else {
        printf("Matches found: %zu\n", matches);
    }
    return 1;
}

static int remove_student(StudentList *records)
{
    if (records->count == 0) {
        puts("No student records to remove.");
        return 1;
    }

    int id;
    if (!read_integer("Student ID to remove: ", &id)) {
        return 0;
    }

    size_t index = 0;
    while (index < records->count && records->students[index].id != id) {
        ++index;
    }
    if (index == records->count) {
        puts("No matching student found. No record removed.");
        return 1;
    }

    /* I move the later records left to fill the removed record's place. */
    for (size_t i = index; i + 1 < records->count; ++i) {
        records->students[i] = records->students[i + 1];
    }
    --records->count;

    if (records->count == 0) {
        free(records->students);
        records->students = NULL;
    } else {
        Student *resized_students = realloc(records->students,
                                            records->count * sizeof(Student));
        if (resized_students != NULL) {
            records->students = resized_students;
        } else {
            fputs("Could not shrink the array. The removal is saved; "
                  "the existing allocation is kept.\n", stderr);
        }
    }

    printf("Student %d removed. Total records: %zu\n", id, records->count);
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

    char student_id[80];
    if (!read_line("Your student ID (for the output header): ",
                   student_id, sizeof(student_id))) {
        puts("\nInput ended. No records loaded.");
        return EXIT_SUCCESS;
    }

    StudentList records = {NULL, initial_count};
    records.students = malloc(sizeof(initial_students));
    if (records.students == NULL) {
        fprintf(stderr, "Could not allocate memory for student records.\n");
        return EXIT_FAILURE;
    }

    memcpy(records.students, initial_students, sizeof(initial_students));

    printf("\nStudent ID: %s\n", student_id);
    puts("Project: Dynamic Academic Performance and Risk Analysis Engine");
    puts("Custom Analysis: Academic Risk Classification");
    printf("Initialization complete: %zu student records loaded.\n",
           records.count);

    /* I use one function-pointer type for all nine menu operations. */
    int (*operations[])(StudentList *) = {
        add_student,
        display_students,
        sort_students,
        calculate_average,
        find_top_student,
        search_student,
        remove_student,
        custom_analysis,
        generate_summary_report
    };

    while (1) {
        puts("\n1. Add a student\n2. Display students\n3. Sort students\n"
             "4. Calculate average grade\n5. Find top-performing students\n"
             "6. Search students\n7. Remove a student\n"
             "8. Run academic risk analysis\n9. Display summary report\n10. Exit");
        int choice;
        if (!read_integer("Choice: ", &choice)) {
            puts("\nInput ended. Closing the program.");
            break;
        }

        if (choice == 10) {
            break;
        }
        if (choice < 1 || choice > 9) {
            puts("Choose an operation from 1 to 10.");
            continue;
        }

        if (!operations[choice - 1](&records)) {
            puts("\nInput ended. Any incomplete operation was cancelled.");
            break;
        }
    }

    free(records.students);
    return EXIT_SUCCESS;
}
