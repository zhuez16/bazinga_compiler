CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(bazinga_compiler)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wno-deprecated-register")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wno-deprecated-register")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
set(CMAKE_CXX_STANDARD 11)

SET(MID_LEVEL_DIR "${CMAKE_SOURCE_DIR}/tmp")
SET(MID_LEVEL_HEADER "${MID_LEVEL_DIR}/include")
file(MAKE_DIRECTORY ${MID_LEVEL_DIR})
file(MAKE_DIRECTORY ${MID_LEVEL_HEADER})

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

FLEX_TARGET(lexer ${CMAKE_SOURCE_DIR}/src/lexer/lexical_analyzer.l ${MID_LEVEL_DIR}/lex1.cpp)
FLEX_TARGET(lexer2 ${CMAKE_SOURCE_DIR}/src/parser/analyzer.l ${MID_LEVEL_DIR}/lex2.cpp)
BISON_TARGET(parser ${CMAKE_SOURCE_DIR}/src/parser/parser.y ${MID_LEVEL_DIR}/bs_parser.cpp DEFINES_FILE ${MID_LEVEL_DIR}/include/bs_parser.h COMPILE_FLAGS --debug)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/IR)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/codegen)
INCLUDE_DIRECTORIES(${MID_LEVEL_DIR}/include)
ADD_SUBDIRECTORY(src/IR)
ADD_SUBDIRECTORY(src/runtime)
ADD_EXECUTABLE(lexer
        src/test/test_lexer.cpp
        ${FLEX_lexer_OUTPUTS}
        )
ADD_EXECUTABLE(parser
        src/test/test_parser.cpp
        src/ast.cpp
        src/IR
        src/IRbuilder
        ${FLEX_lexer2_OUTPUTS}
        ${BISON_parser_OUTPUTS})
ADD_EXECUTABLE(builder
        src/pass/CFG_simply.cpp
        src/pass/dominator.cpp
        src/pass/loop_expansion.cpp
        src/pass/mem2reg.cpp
        src/pass/loop_search.cpp
        src/pass/global2local.cpp
        src/IRbuilder/bzcompiler.cpp
        src/IRbuilder/bzcompiler_builder.cpp
        ${FLEX_lexer2_OUTPUTS}
        ${BISON_parser_OUTPUTS}
        src/ast.cpp
        src/pass_manager.cpp
        src/pass/active_vars.cpp
        src/pass/SCPcombineDCE.cpp
        src/pass/CFG.cpp
        src/pass/Sink.cpp
        src/pass/GVN.cpp
        src/pass/CodeElimination.cpp)
#ADD_EXECUTABLE(codegen
#       src/codegen/codegen.cpp
#        src/codegen/instgen.cpp
#        src/codegen/regAlloc.cpp
#        src/pass/dominator.cpp
#        src/pass/mem2reg.cpp
#        src/pass/loop_search.cpp
#        src/pass/global2local.cpp
#        src/IRbuilder/bzcompiler.cpp
#        src/IRbuilder/bzcompiler_builder.cpp
#        ${FLEX_lexer2_OUTPUTS}
#        ${BISON_parser_OUTPUTS}
#        src/ast.cpp
#        src/pass_manager.cpp
#        src/pass/active_vars.cpp
#        src/pass/SCPcombineDCE.cpp
#        )
TARGET_LINK_LIBRARIES(
        builder IR_lib
)
#TARGET_LINK_LIBRARIES(
#        codegen IR_lib
#)