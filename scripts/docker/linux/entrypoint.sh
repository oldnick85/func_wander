#!/bin/bash
set -e

# Parse command line arguments
ACTION=${1:-"build"}
BUILD_TYPE=${2:-"Release"}
BUILD_DIR=${3:-"build-linux"}

echo "Action: $ACTION, Build type: $BUILD_TYPE, Build directory: $BUILD_DIR"

case $ACTION in
    "build")
        echo "Building project for Linux (Ubuntu 25.10)..."

        conan profile detect --force

        cd /workspace

        # Create build directory and navigate to it
        mkdir -p $BUILD_DIR
        
        # Install Conan dependencies
        conan install . \
            --build=missing \
            --output-folder=$BUILD_DIR \
            -s compiler=gcc \
            -s compiler.version=15 \
            -s compiler.libcxx=libstdc++11 \
            -s build_type=$BUILD_TYPE \
            -s compiler.cppstd=23        

        cd $BUILD_DIR
        
        # Configure CMake project
        cmake .. \
            -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
            -DCMAKE_CXX_STANDARD=23 \
            -DCMAKE_CXX_STANDARD_REQUIRED=ON \
            -DCMAKE_CXX_EXTENSIONS=OFF \
            -DCMAKE_CXX_COMPILER=g++-15 \
            -DCMAKE_C_COMPILER=gcc-15
        
        # Build the project
        cmake --build . --config $BUILD_TYPE
        
        # Copy executables to bin directory
        #mkdir -p ../bin/linux
        #find . -name "*.exe" -o -name "yggdrasil*" -type f -executable -exec cp {} ../bin/linux/ \;
        ;;
    
    "test")
        echo "Running unit tests for Linux..."

        cd /workspace
        cd $BUILD_DIR
        
        # Run tests with CTest
        ctest --output-on-failure -C $BUILD_TYPE -V
        ;;
    
    "format")
        echo "Checking code formatting with clang-format..."
        
        # Check formatting without making changes
        find src test -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | \
            xargs clang-format-20 --dry-run --Werror --style=file
        
        echo "✅ All files are properly formatted!"
        ;;
    
    "all")
        echo "Running full CI pipeline..."
        /usr/local/bin/entrypoint.sh format
        /usr/local/bin/entrypoint.sh build $BUILD_TYPE $BUILD_DIR
        /usr/local/bin/entrypoint.sh test $BUILD_TYPE $BUILD_DIR
        ;;
    
    *)
        echo "Available actions: build, test, format, all"
        echo "Usage: entrypoint.sh [action] [build_type] [build_dir]"
        exit 1
        ;;
esac