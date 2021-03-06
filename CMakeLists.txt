CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(bazinga_compiler)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wno-deprecated-register")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wno-deprecated-register")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
set(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/IR)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/codegen)
INCLUDE_DIRECTORIES(${MID_LEVEL_DIR}/include)
ADD_SUBDIRECTORY(src/IR)
ADD_EXECUTABLE(compiler
        # As we can't use a CMakeLists in submit, just copy the code generated by bison / flex to src folder.
        src/bs_parser.cpp
        src/lex2.cpp
        # Main function
        src/compiler.cpp
        # Front end
        src/IRbuilder/bzcompiler_builder.cpp
        src/ast.cpp
        # Back end
        src/ASMIR/ASMBuilder.cpp
        src/ASMIR/ASMPrinter.cpp
        src/ASMIR/ASValue.cpp
        src/ASMIR/RegAllocMapper.cpp
        src/ASMIR/SsaAsmPrinter.cpp
        src/codegen/linearScanOnSSA.cpp
        src/ASMIR/BBOrderGenerator.cpp
        # Opt/Analysis Pass
        src/pass_manager.cpp
        src/pass/active_vars.cpp
        src/pass/CFG.cpp
        src/pass/CFG_simply.cpp
        src/pass/CodeElimination.cpp
        src/pass/dominator.cpp
        src/pass/get_ptr_simply.cpp
        src/pass/global2local.cpp
        src/pass/GVN.cpp
        src/pass/Inline.cpp
        src/pass/loop_expansion.cpp
        src/pass/loop_search.cpp
        src/pass/mem2reg.cpp
        src/pass/power_sum.cpp
        src/pass/reassociate.cpp
        src/pass/SCPcombineDCE.cpp
        src/pass/Sink.cpp
        include/ASMIR/BBOrderGenerator.h src/ASMIR/BBOrderGenerator.cpp)
TARGET_LINK_LIBRARIES(
        compiler IR_lib
)