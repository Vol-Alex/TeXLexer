project (TeXLexer)

set(CMAKE_CXX_STANDARD 14)

include_directories(PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE SRC ${CMAKE_SOURCE_DIR}/src/*.*)

ADD_CUSTOM_COMMAND(
   SOURCE ${CMAKE_SOURCE_DIR}/src/LexerImpl.l
   COMMAND flex 
   ARGS -o ${CMAKE_BINARY_DIR}/src/LexerImpl.c
           ${CMAKE_SOURCE_DIR}/src/LexerImpl.l
   TARGET LexerImpl
   OUTPUTS ${CMAKE_BINARY_DIR}/src/LexerImpl.c)

SET(SRC ${SRC} ${CMAKE_BINARY_DIR}/src/LexerImpl.c)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/src/LexerImpl.c GENERATED)

add_library(TeXLexer ${SRC})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
