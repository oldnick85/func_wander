# Func Wander 🔍🧮✨

*A brute-force approach to discovering functions that satisfy given constraints*

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.28%2B-brightgreen.svg)](https://cmake.org)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 📖 Overview

Func Wander is a C++23 header-only library that explores the space of mathematical functions through systematic enumeration. It can discover novel function implementations that satisfy specific constraints—even when traditional mathematical approaches fall short.

Think of it as **function synthesis from examples**. Given a set of basic operations and desired input-output pairs, it searches for expressions that match the target behavior.

## ✨ Features

- 🔬 **Systematic enumeration** of function expressions
- 🌳 **Tree-based representation** of mathematical expressions
- ⚡ **Caching and optimization** for performance
- 🔄 **Parallel search** with `std::jthread`
- 💾 **State persistence** via JSON serialization
- 🎯 **Customizable targets** and distance metrics
- 🧩 **Extensible atomic function** system

## 🏗️ Architecture

```text
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Atomic          │    │ Function        │    │ Search          │
│ Functions       │───▶│ Trees           │───▶│ Engine          │
│ (+, sin, etc.)  │    │ (Expression)    │    │ (Brute-force)   │
└─────────────────┘    └─────────────────┘    └─────────┬───────┘
                                                        │
                                                        ▼
┌─────────────────┐                            ┌─────────────────┐
│ Target          │                            │ Results         │
│ Specification   │◀───────────────────────────│ & Ranking       │
│ (Desired I/O)   │                            └─────────────────┘
└─────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- C++23 compatible compiler (GCC 13+ or Clang 16+)
- CMake 3.28+
- Python and Conan for dependency management

### Installation

```bash
# Create virtual environment
python3 -m venv env
source env/bin/activate

# Install Conan
pip install conan

# Detect compiler and setup profile
conan profile detect --force

# Install dependencies
conan install . --output-folder=build --build=missing

# Build with CMake
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## ⚠️ Limitations

 - Curse of Dimensionality 📊: Higher-dimensional problems require exponentially more computation
 - Operation Set Bound 🧩: Can only discover functions using provided atomic operations
 - No Optimality Guarantee 🎯: May not find simplest/most elegant solution
 - Memory Intensive 🐘: Large search spaces require significant memory


📚 Documentation

For detailed API documentation, run:
```bash
# Generate Doxygen documentation
doxygen Doxyfile
# Open docs/html/index.html in your browser
```

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
