# We need thread support
find_package(Threads REQUIRED)

# Enable ExternalProject CMake module
include(ExternalProject)

# Download and install GoogleTest
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/lib/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

# Set libgmock properties
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/lib/libgmock.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/googletest/include"
                    "${source_dir}/googlemock/include")

file(GLOB TEX_FILES ${CMAKE_CURRENT_SOURCE_DIR}/files/*.tex)

list(APPEND TEX_FILES_AS_CPP "")
foreach(FILE ${TEX_FILES})
    list(APPEND TEX_FILES_AS_CPP "R\"(${FILE})\"")
endforeach()
string(REPLACE ";" ",\n" TEX_FILES_TO_CONFIG "${TEX_FILES_AS_CPP}")
configure_file(MathMLGeneratorTestSuite.cpp.in MathMLGeneratorTestSuite.cpp)

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.*)
list(APPEND SRC ${CMAKE_CURRENT_BINARY_DIR}/MathMLGeneratorTestSuite.cpp)

add_executable(test ${SRC})
add_dependencies(test gtest)
target_link_libraries(test LINK_PUBLIC
    TeXLexer
    libgtest
    libgmock
)
