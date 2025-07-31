# Func Wander

A brute-force approach to finding functions that satisfy given constraints

## Overview

This project explores the space of possible mathematical functions to discover implementations that satisfy specific constraints. By systematically generating and testing functions, it can discover novel solutions to problems where traditional mathematical approaches fall short.

## Requirements

C++23 compatible compiler: GCC or Clang, CMake 3.28+, Python and Conan.

## Build instructions

### Ubuntu 24.04+

```bash
python3 -m venv env
source env/bin/activate
pip install conan

conan profile detect --force

conan install . --output-folder=build --build=missing
```

## Limitations

 * Curse of dimensionality - higher-dimensional problems require exponentially more computation. 
 * Can only discover functions within the defined operation set. 
 * No guarantee of finding simplest/most elegant solution.

