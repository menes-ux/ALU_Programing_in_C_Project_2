#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

enum { TEMPERATURE = 1, PRESSURE, BATTERY };

typedef union {
    float temperature;
    int pressure;
    float voltage;
} Reading;

typedef struct Device {
    int id;
    char name[40];
    int type;
    Reading reading;
    Reading previous;
    int has_reading;
    int has_previous;
    int status;                 /* 0 = normal, 1 = alert. */
    int range_alert;
    int drift_alert;
    float change;
} Device;

/* Store the ID rather than a device pointer: realloc can move the devices. */
typedef struct {
    int device_id;
    Reading reading;
} PendingReading;

typedef struct {
    Device *devices;
    size_t count;
    PendingReading *pending;
    size_t pending_count;
    size_t processed;
    size_t normal;
    size_t abnormal;
    size_t alerts_by_type[3];
    size_t drift_comparisons;
    size_t drift_alerts;
} MonitoringSystem;

static const char *type_names[] = {"Temperature", "Pressure", "Battery voltage"};
static const char *units[] = {"C", "kPa", "V"};
static const char *monitor_names[] = {
    "temperature_monitor", "pressure_monitor", "battery_monitor"
};

/* Project operating limits, inclusive at both ends. */
static const float minimums[] = {0.0f, 90.0f, 3.3f};
static const float maximums[] = {40.0f, 110.0f, 4.2f};
static const float drift_limits[] = {8.0f, 12.0f, 0.4f};

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

static int read_integer(const char *prompt, int *result)
{
    char input[80];
    while (read_line(prompt, input, sizeof(input))) {
        char *end;
        errno = 0;
        long value = strtol(input, &end, 10);
        if (errno == ERANGE || end == input || *end != '\0' ||
            value < INT_MIN || value > INT_MAX) {
            puts("Enter a valid whole number within the integer range.");
            continue;
        }
        *result = (int)value;
        return 1;
    }
    return 0;
}

static Device *find_device(MonitoringSystem *system, int id)
{
    Device *device = system->devices;
    for (size_t i = 0; i < system->count; ++i, ++device) {
        if (device->id == id) {
            return device;
        }
    }
    return NULL;
}

/* Only read the union member belonging to this device's type. */
static float reading_value(int type, const Reading *reading)
{
    switch (type) {
        case TEMPERATURE:
            return reading->temperature;
        case PRESSURE:
            return (float)reading->pressure;
        case BATTERY:
            return reading->voltage;
        default:
            return 0.0f;
    }
}

static void print_reading(const Device *device, const Reading *reading)
{
    printf("ID %d | %s | %s | %.2f %s", device->id, device->name,
           type_names[device->type - 1], reading_value(device->type, reading),
           units[device->type - 1]);
}

static void temperature_monitor(Device *device)
{
    if (device->reading.temperature < minimums[TEMPERATURE - 1] ||
        device->reading.temperature > maximums[TEMPERATURE - 1]) {
        device->range_alert = 1;
        device->status = 1;
    }
}

static void pressure_monitor(Device *device)
{
    if (device->reading.pressure < minimums[PRESSURE - 1] ||
        device->reading.pressure > maximums[PRESSURE - 1]) {
        device->range_alert = 1;
        device->status = 1;
    }
}

static void battery_monitor(Device *device)
{
    if (device->reading.voltage < minimums[BATTERY - 1] ||
        device->reading.voltage > maximums[BATTERY - 1]) {
        device->range_alert = 1;
        device->status = 1;
    }
}

/* Sensor Drift Detection compares consecutive readings of the SAME device.
   Change = current reading - previous reading, made positive.
   Alert when the change exceeds 8 C, 12 kPa, or 0.40 V for that device type.
   The first reading is only a baseline because there is no previous value.
   Even two readings inside the normal range can cause a drift alert. */
static void sensor_drift_detection(Device *device)
{
    if (!device->has_previous) {
        return;
    }
    device->change = reading_value(device->type, &device->reading) -
                     reading_value(device->type, &device->previous);
    if (device->change < 0.0f) {
        device->change = -device->change;
    }
    /* The tolerance avoids false alerts from decimal float rounding.
       It is much smaller than the smallest simulated step of 0.01 V. */
    if (device->change > drift_limits[device->type - 1] + 0.00001f) {
        device->drift_alert = 1;
        device->status = 1;
    }
}

static void process_device(Device *device, void (*callback)(Device *))
{
    callback(device);
}

static void print_device_alerts(const Device *device)
{
    int type = device->type - 1;
    if (device->range_alert) {
        print_reading(device, &device->reading);
        printf(" | ALERT: outside operating range %.2f to %.2f %s"
               " | Callback: %s\n",
               minimums[type], maximums[type], units[type], monitor_names[type]);
    }
    if (device->drift_alert) {
        print_reading(device, &device->reading);
        printf(" | ALERT: change %.2f %s from previous %.2f %s;"
               " maximum allowed change %.2f %s"
               " | Callback: sensor_drift_detection\n",
               device->change, units[type], reading_value(device->type, &device->previous),
               units[type], drift_limits[type], units[type]);
    }
}

/* Menu operations return 0 when input ends, or 1 to continue. */
static int register_device(MonitoringSystem *system)
{
    Device device = {0};
    while (1) {
        if (!read_integer("Device ID: ", &device.id)) {
            return 0;
        }
        if (device.id <= 0) {
            puts("Device ID must be positive.");
        } else if (find_device(system, device.id) != NULL) {
            puts("This device ID already exists. Choose another ID.");
        } else {
            break;
        }
    }
    if (!read_line("Device name: ", device.name, sizeof(device.name))) {
        return 0;
    }
    while (1) {
        if (!read_integer("Type (1 = Temperature, 2 = Pressure, 3 = Battery): ",
                          &device.type)) {
            return 0;
        }
        if (device.type >= TEMPERATURE && device.type <= BATTERY) {
            break;
        }
        puts("Choose device type 1, 2, or 3.");
    }
    if (system->count >= (size_t)-1 / sizeof(Device)) {
        fputs("Cannot allocate space for another device.\n", stderr);
        return 1;
    }
    Device *resized = realloc(system->devices, (system->count + 1) * sizeof(Device));
    if (resized == NULL) {
        fputs("Could not register device. Existing devices are kept.\n", stderr);
        return 1;
    }
    system->devices = resized;
    system->devices[system->count++] = device;
    printf("Device registered. Total devices: %zu\n", system->count);
    return 1;
}

static int display_devices(MonitoringSystem *system)
{
    if (system->count == 0) {
        puts("No registered devices.");
        return 1;
    }
    puts("\nRegistered devices (latest processed readings):");
    Device *device = system->devices;
    for (size_t i = 0; i < system->count; ++i, ++device) {
        if (device->has_reading) {
            print_reading(device, &device->reading);
            printf(" | %s\n", device->status ? "ALERT" : "NORMAL");
        } else {
            printf("ID %d | %s | %s | Awaiting first reading\n",
                   device->id, device->name, type_names[device->type - 1]);
        }
    }
    printf("Total devices: %zu\n", system->count);
    return 1;
}

static int remove_device(MonitoringSystem *system)
{
    if (system->count == 0) {
        puts("No registered devices to remove.");
        return 1;
    }
    int id;
    if (!read_integer("Device ID to remove: ", &id)) {
        return 0;
    }
    Device *device = find_device(system, id);
    if (device == NULL) {
        puts("No matching device found.");
        return 1;
    }
    size_t index = (size_t)(device - system->devices);
    for (size_t i = index; i + 1 < system->count; ++i) {
        system->devices[i] = system->devices[i + 1];
    }
    --system->count;

    /* Discard this device's unprocessed readings. Reusing its ID later must
       not attach an old reading to a newly registered device. */
    size_t kept = 0;
    for (size_t i = 0; i < system->pending_count; ++i) {
        if (system->pending[i].device_id != id) {
            system->pending[kept++] = system->pending[i];
        }
    }
    size_t discarded = system->pending_count - kept;
    system->pending_count = kept;
    if (kept == 0) {
        free(system->pending);
        system->pending = NULL;
    }

    if (system->count == 0) {
        free(system->devices);
        system->devices = NULL;
    } else {
        Device *resized = realloc(system->devices, system->count * sizeof(Device));
        if (resized != NULL) {
            system->devices = resized;
        } else {
            fputs("Could not shrink memory. Device removed; existing allocation kept.\n",
                  stderr);
        }
    }
    printf("Device %d removed. Total devices: %zu. Pending readings discarded: %zu\n",
           id, system->count, discarded);
    return 1;
}

static int generate_readings(MonitoringSystem *system)
{
    if (system->count == 0) {
        puts("Register a device before generating readings.");
        return 1;
    }
    if (system->pending_count != 0) {
        puts("Process the pending readings before generating another batch.");
        return 1;
    }
    /* Visit every registered device, repeating devices when fewer than 12
       exist. This gives at least 12 readings and covers all initial types. */
    size_t count = system->count < 12 ? 12 : system->count;
    if (count > (size_t)-1 / sizeof(PendingReading)) {
        fputs("The reading batch is too large to allocate.\n", stderr);
        return 1;
    }
    PendingReading *pending = malloc(count * sizeof(PendingReading));
    if (pending == NULL) {
        fputs("Could not allocate memory for readings.\n", stderr);
        return 1;
    }
    puts("\nGenerated readings:");
    for (size_t i = 0; i < count; ++i) {
        Device *device = system->devices + i % system->count;
        pending[i].device_id = device->id;
        /* Simulation ranges: -10.0 to 60.0 C in 0.1 C steps;
           70 to 130 kPa in whole units; 2.50 to 4.50 V in 0.01 V steps. */
        switch (device->type) {
            case TEMPERATURE:
                pending[i].reading.temperature = (rand() % 701 - 100) / 10.0f;
                break;
            case PRESSURE:
                pending[i].reading.pressure = 70 + rand() % 61;
                break;
            case BATTERY:
                pending[i].reading.voltage = (250 + rand() % 201) / 100.0f;
                break;
        }
        print_reading(device, &pending[i].reading);
        puts(" | Pending processing");
    }
    system->pending = pending;
    system->pending_count = count;
    printf("%zu readings generated. Choose option 5 to process them.\n", count);
    return 1;
}

static int process_readings(MonitoringSystem *system)
{
    if (system->pending_count == 0) {
        puts("No pending readings. Generate readings first.");
        return 1;
    }
    void (*monitors[])(Device *) = {
        temperature_monitor, pressure_monitor, battery_monitor
    };
    puts("\nProcessing readings through device and drift callbacks:");
    for (size_t i = 0; i < system->pending_count; ++i) {
        Device *device = find_device(system, system->pending[i].device_id);
        if (device == NULL) {
            continue;
        }
        device->has_previous = device->has_reading;
        if (device->has_previous) {
            device->previous = device->reading;
            ++system->drift_comparisons;
        }
        device->reading = system->pending[i].reading;
        device->status = 0;
        device->range_alert = 0;
        device->drift_alert = 0;
        device->change = 0.0f;

        /* Select the type-specific callback, then run the custom callback.
           Both are invoked through a function pointer by process_device. */
        process_device(device, monitors[device->type - 1]);
        process_device(device, sensor_drift_detection);
        device->has_reading = 1;
        ++system->processed;
        if (device->status) {
            ++system->abnormal;
        } else {
            ++system->normal;
        }
        /* One reading can cause two alert events: range and drift. It still
           counts as just one abnormal reading in the summary. */
        system->alerts_by_type[device->type - 1] +=
            (size_t)(device->range_alert + device->drift_alert);
        system->drift_alerts += (size_t)device->drift_alert;

        print_reading(device, &device->reading);
        printf(" | %s | Callbacks: %s, sensor_drift_detection\n",
               device->status ? "ABNORMAL" : "NORMAL",
               monitor_names[device->type - 1]);
        if (!device->has_previous) {
            puts("  Drift check: first reading establishes this device's baseline.");
        }
        print_device_alerts(device);
    }
    free(system->pending);
    system->pending = NULL;
    system->pending_count = 0;
    return 1;
}

static int display_alerts(MonitoringSystem *system)
{
    puts("\nLatest alerts for currently registered devices:");
    size_t affected = 0;
    const Device *device = system->devices;
    for (size_t i = 0; i < system->count; ++i, ++device) {
        if (device->has_reading && device->status) {
            print_device_alerts(device);
            ++affected;
        }
    }
    if (affected == 0) {
        puts("No alerts in the latest processed device readings.");
    }
    return 1;
}

static int generate_summary(MonitoringSystem *system)
{
    puts("\nMonitoring Summary");
    printf("Registered devices: %zu\nPending readings: %zu\n",
           system->count, system->pending_count);
    puts("Session totals include readings from devices that were later removed.");
    printf("Total readings processed: %zu\nNormal readings: %zu\nAbnormal readings: %zu\n",
           system->processed, system->normal, system->abnormal);
    puts("Alert events by device type (range and drift each count as one event):");
    for (int i = 0; i < 3; ++i) {
        printf("  %s: %zu\n", type_names[i], system->alerts_by_type[i]);
    }
    puts("Custom Callback: Sensor Drift Detection");
    puts("Absolute change between consecutive readings of the same device.");
    puts("Alert thresholds: change > 8 C, > 12 kPa, or > 0.40 V.");
    printf("Drift comparisons: %zu\nDrift alerts: %zu\n",
           system->drift_comparisons, system->drift_alerts);
    return 1;
}

int main(void)
{
    char student_id[80];
    if (!read_line("Your student ID: ", student_id, sizeof(student_id))) {
        return EXIT_SUCCESS;
    }
    const Device initial_devices[] = {
        {.id = 101, .name = "Room temperature", .type = TEMPERATURE},
        {.id = 102, .name = "Air pressure", .type = PRESSURE},
        {.id = 103, .name = "Battery pack", .type = BATTERY}
    };
    MonitoringSystem system = {0};
    system.count = sizeof(initial_devices) / sizeof(initial_devices[0]);
    system.devices = malloc(sizeof(initial_devices));
    if (system.devices == NULL) {
        fputs("Could not allocate memory for devices.\n", stderr);
        return EXIT_FAILURE;
    }
    memcpy(system.devices, initial_devices, sizeof(initial_devices));
    srand((unsigned int)time(NULL));

    printf("\nStudent ID: %s\n", student_id);
    puts("Project: Dynamic IoT Event Monitoring and Callback Engine");
    puts("Custom Callback: Sensor Drift Detection");
    puts("Simulation ranges: temperature -10 to 60 C; pressure 70 to 130 kPa;");
    puts("battery voltage 2.50 to 4.50 V.");
    puts("Normal ranges: temperature 0 to 40 C; pressure 90 to 110 kPa;");
    puts("battery voltage 3.30 to 4.20 V. Range endpoints are normal.");
    puts("Drift alert: change > 8 C, > 12 kPa, or > 0.40 V between readings.");
    puts("Three sample devices are registered. Generate readings, then process them.");

    int (*operations[])(MonitoringSystem *) = {
        register_device, display_devices, remove_device, generate_readings,
        process_readings, display_alerts, generate_summary
    };
    while (1) {
        puts("\n1. Register a device\n2. Display registered devices\n3. Remove a device\n"
             "4. Generate simulated readings\n5. Process pending readings\n"
             "6. Display latest alerts\n7. Generate monitoring summary\n8. Exit");
        int choice;
        if (!read_integer("Choice: ", &choice) || choice == 8) {
            break;
        }
        if (choice < 1 || choice > 7) {
            puts("Choose an operation from 1 to 8.");
            continue;
        }
        if (!operations[choice - 1](&system)) {
            break;
        }
    }
    free(system.pending);
    free(system.devices);
    return EXIT_SUCCESS;
}
