# Sorting Studio

Sorting Studio is a local C++ application for learning about and comparing sorting algorithms. Its browser-based interface visualizes each algorithm's trace and reports runtime and operation counts.

## Features

- Learn mode with algorithm pseudocode, explanations, complexity details, and an animated trace.
- Compare two algorithms against the same dataset.
- Merge Sort, Quick Sort, Heap Sort, Counting Sort, Radix Sort, and Bucket Sort.
- Playback controls for pausing, resuming, restarting, and stepping through a trace.
- Random datasets, manual input, CSV upload, and an included sample CSV.
- Light and dark themes.
- Comparison metrics for elapsed time, comparisons, and swaps or writes.

## Technologies

- C++17 for the sorting engine and HTTP server.
- CMake 3.16 or later for configuring and building.
- HTML, CSS, and vanilla JavaScript for the frontend.
- Native Windows sockets or POSIX sockets; there are no third-party runtime packages.

## Project structure

```text
.
├── .vscode/
│   ├── launch.json
│   └── tasks.json
├── data/
│   └── sample_dataset.csv
├── scripts/
│   └── run.ps1
├── src/
│   └── main.cpp
├── web/
│   ├── app.js
│   ├── index.html
│   └── styles.css
├── CMakeLists.txt
├── README.md
└── run.ps1
```

Generated build files are written to `build/` and are intentionally not part of the repository.

## Requirements

- CMake 3.16 or later.
- A C++17 compiler (for example, Visual Studio 2022/MSVC on Windows or GCC/Clang on Linux/macOS).
- PowerShell for the convenience launcher on Windows.

## Build and run

From the project directory, configure and build:

```powershell
cmake -S . -B build
cmake --build build
```

On Windows, start the server with:

```powershell
.\build\smart_sorting_studio.exe 8080
```

On Linux or macOS, start the built executable with:

```sh
./build/smart_sorting_studio 8080
```

Then open <http://localhost:8080> in a browser. The optional first argument selects the port; the default is `8080`.

On Windows, the convenience script configures and builds the project, starts the server, and opens the browser:

```powershell
.\scripts\run.ps1
```

If an instance is already running, the script stops with a message; it does not terminate existing processes.

## Environment configuration

The project does not currently use environment variables or require a `.env` file. No credentials or external API keys are needed.

## Usage

1. Open the local site and choose **Learn an Algorithm** or **Compare Algorithms**.
2. Select an algorithm (or pair), then generate a random dataset or provide values.
3. In compare mode, paste values, upload a CSV/text file, or load the included sample dataset.
4. Start the visualization and use the playback controls to inspect the trace.

## Backend endpoints

- `GET /api/health` reports whether the local backend is running.
- `POST /api/sort` returns a sorting trace and metrics for one algorithm.
- `POST /api/compare` returns traces and metrics for two algorithms.

The server binds to the local machine and is intended for local use, not as a public-facing service.

## Possible future improvements

- Add focused automated tests for sorting results and API input handling.
- Add accessibility improvements and keyboard controls for the visualizer.
- Support configurable dataset limits and additional dataset formats.
