# Programming in C — Project 2

Three independent C applications developed for the Programming in C course at African Leadership University.

## 1. Projects

| Project | Application | Purpose |
| --- | --- | --- |
| 1 | Dynamic Academic Performance and Risk Analysis | Manage student records and analyze academic performance. |
| 2 | Dynamic Text Mining and Linguistic Pattern Analyzer | Analyze word frequency and vocabulary in a text. |
| 3 | Dynamic IoT Event Monitoring and Callback Engine | Simulate sensor readings and process alerts using callbacks. |

## 2. Repository Structure

```text
ALU_Programing_in_C_Project_2/
├── README.md
├── .gitignore
├── project-1-academic-analysis/
│   ├── src/main.c
│   ├── tests/
│   └── README.md
├── project-2-text-mining/
│   ├── src/main.c
│   ├── data/sample_text.txt
│   ├── tests/
│   └── README.md
├── project-3-iot-monitoring/
│   ├── src/main.c
│   ├── tests/
│   └── README.md
└── submission/
    ├── screenshots/
    └── report/
```

`src/` contains the C program. `data/` contains the input text for Project 2. `tests/` is optional and can remain empty because the applications can be tested manually. `submission/` is reserved for final output evidence and the report.

## 3. Project 1 — Academic Analysis

The program uses dynamically allocated student records to support adding, displaying, sorting, searching, and removing students. It calculates grades and produces an academic summary.

**Custom analysis:** Academic Risk Classification.

| Grade | Category |
| --- | --- |
| 80–100 | Excellent |
| 60–79 | Satisfactory |
| 50–59 | Needs Improvement |
| 0–49 | At Risk |

**Flow:** Student records → Grade analysis → Risk classification → Summary

## 4. Project 2 — Text Mining

The program reads text, counts total and distinct words, tracks frequencies, searches and sorts words, and reports memory addresses.

**Sample source:** *Nineteen Eighty-Four (1984)* by George Orwell (1949), Part One, Chapter 1.

**Custom analysis:** Lexical Diversity Score.

```text
Lexical diversity (%) = (distinct words / total words) × 100
```

**Flow:** Text input → Word extraction → Frequency analysis → Summary

## 5. Project 3 — IoT Monitoring

The program simulates temperature, pressure, and battery-voltage devices. It manages registered devices, generates readings, dispatches monitoring callbacks, and reports alerts.

**Planned custom callback:** Sensor Drift Detection, which compares consecutive readings against a documented change threshold.

**Flow:** Device → Simulated reading → Monitoring callback → Normal/alert status → Summary

**Note:** Project 3 details should be updated to match the final implementation.
