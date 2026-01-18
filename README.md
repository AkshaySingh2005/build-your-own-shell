# Shell 

A minimal shell implementation in C++23 with line editing via GNU Readline. Built with CMake .

## Requirements
- CMake ≥ 3.13
- A C++ compiler with C++23 support (e.g., g++ 13+, clang 16+)
- GNU Readline development headers
	- Ubuntu/Debian: `sudo apt-get install build-essential cmake libreadline-dev`
	- Arch: `sudo pacman -S base-devel cmake readline`
	- Fedora: `sudo dnf install gcc-c++ cmake readline-devel`

## Build
```bash
cmake -B build -S .
cmake --build build
```

## Run
```bash
./build/shell
```

## Project Layout
- [src/](src): Shell source files
- [CMakeLists.txt](CMakeLists.txt): Build configuration
- [.gitignore](.gitignore): Ignore build outputs and editor files
- [.gitattributes](.gitattributes): Optional git attributes

## Notes
- The target links against `readline` (`target_link_libraries(shell PRIVATE readline)` in [CMakeLists.txt](CMakeLists.txt)). Ensure the dev package is installed.
- If you prefer vcpkg, you can add it later; this repository uses plain CMake by default.

