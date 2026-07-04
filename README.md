# Smart Resume Screening System (C++)

A menu-driven DSA project that stores resumes, extracts skills, matches candidates to a job, ranks them, and generates shortlist reports.

## DSA concepts used

- **Hash table:** `std::unordered_map` provides average O(1) resume insertion, deletion, and ID lookup.
- **Trie:** detects known technical and soft skills in resume text efficiently.
- **Sorting/ranking:** candidates are sorted by match score, experience, then name.
- **Set:** removes duplicate extracted skills and supports matching.

The score is **80% skill match + 20% experience match**. Experience receives full credit when the candidate meets the minimum requirement.

## Build and run

## Run the web interface

### Easiest method

Double-click **`run_web.bat`**. Your browser will open automatically. Keep the black server window open while using the application.

### PowerShell method

Compile the web server with MinGW:

```powershell
g++ -std=c++17 -Iinclude src/web_server.cpp src/screening_system.cpp -lws2_32 -o resume_web.exe
.\resume_web.exe
```

Keep that PowerShell window open, then visit **http://localhost:8080** in your browser. Press `Ctrl+C` in PowerShell when you want to stop it.

The web application includes candidate management, automatic skill extraction, search, job matching, ranked results, and shortlist export.

### CMake (recommended)

```powershell
cmake -S . -B build
cmake --build build
.\build\Debug\resume_screening.exe
```

With MinGW, the executable is usually at `build\resume_screening.exe`.

### Directly with g++

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude src/main.cpp src/screening_system.cpp -o resume_screening.exe
.\resume_screening.exe
```

## Quick demo

1. Select option **5** and load `data/sample_resumes.txt`.
2. Select option **7**.
3. Enter a job such as `C++ Developer`, minimum experience `3`, and skills `C++, data structures, algorithms, SQL, Git`.
4. Use option **8** to save a ranked shortlist as a text report.

## Resume file format

Each resume occupies one line with five pipe-separated fields:

```text
name|email|experience years|education|resume text
```

Do not place the `|` character inside a field. The built-in trie recognizes the skills listed in `ScreeningSystem`'s constructor; add more entries there to expand the vocabulary.
