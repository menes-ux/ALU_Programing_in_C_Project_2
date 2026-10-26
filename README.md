# Programming in C - Project 2

This repository contains my three applications for the Programming in C assignment. I will build and test each application separately.

## Applications

- **Academic analysis:** manage student records and analyse academic performance.
- **Text mining:** analyse a published paragraph and calculate word statistics.
- **IoT monitoring:** simulate sensor readings and process alerts with callbacks.

## Repository structure

```text
.
|-- README.md
|-- .gitignore
|-- project-1-academic-analysis/
|   |-- src/main.c
|   |-- tests/
|   `-- README.md
|-- project-2-text-mining/
|   |-- src/main.c
|   |-- data/
|   |-- tests/
|   `-- README.md
|-- project-3-iot-monitoring/
|   |-- src/main.c
|   |-- tests/
|   `-- README.md
`-- submission/
    |-- screenshots/
    `-- report/
```

The `src` folders will contain the C code. The `tests` folders will contain test inputs and checks. The text mining `data` folder will hold the selected paragraph and its source details.

The `submission` folder will hold execution evidence and the final report. The `.gitkeep` files keep empty folders in Git until I add their content.

## Development

I will use C11 and GCC with these compiler options:

```text
-std=c11 -Wall -Wextra -Wpedantic
```

Only the repository structure is ready. The source files are placeholders. I will add compile and run instructions when each application is implemented.
