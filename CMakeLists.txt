project (TeXLexer)

cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 14)

include_directories(PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE SRC ${CMAKE_SOURCE_DIR}/src/*.*)
file(GLOB_RECURSE HDR ${CMAKE_SOURCE_DIR}/src/*.h)

if(TXL_GENERATE_LEXER)
    add_custom_target(
        LexerImpl
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/src
        COMMAND flex -o ${CMAKE_BINARY_DIR}/src/LexerImpl.c
                        ${CMAKE_SOURCE_DIR}/src/LexerImpl.l)

    set(SRC ${SRC} ${CMAKE_BINARY_DIR}/src/LexerImpl.c)
    set_source_files_properties(${CMAKE_BINARY_DIR}/src/LexerImpl.c GENERATED)
endif()

add_library(TeXLexer ${SRC})
set_target_properties(TeXLexer PROPERTIES PUBLIC_HEADER "${HDR}")
if(TARGET LexerImpl)
    add_dependencies(TeXLexer LexerImpl)
endif()

if(NOT TXL_BUILD_LIB_ONLY)
    add_executable(texToMML texToMML.cpp)
    target_link_libraries(texToMML LINK_PUBLIC
                          TeXLexer
    )

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()

include(GNUInstallDirs)
install(TARGETS TeXLexer
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/TeXLexer")
