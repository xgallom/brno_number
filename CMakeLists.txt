cmake_minimum_required(VERSION 3.15)
project(brno_number)

set(CMAKE_CXX_STANDARD 17)

include_directories(number)

add_executable(brno_number
        number/number.cpp
        number/number.hpp
        number/test.cpp)

target_compile_options(brno_number PUBLIC
        -Werror
        -Wall
        -Wextra # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a
        # parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a
        # non-virtual destructor. This helps catch hard to
        # track down memory errors
        -Wold-style-cast # warn for c-style casts
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual
        # function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output
        # (ie printf)
        )
