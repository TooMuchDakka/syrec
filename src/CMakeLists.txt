add_library(${PROJECT_NAME}
            ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/syrec_synthesis.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/syrec_synthesis_additional_lines.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/syrec_synthesis_no_additional_lines.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/simulation/simple_simulation.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/dd_synthesis.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/encoding.cpp

            ${${PROJECT_NAME}_SOURCE_DIR}/include/algorithms/simulation/simple_simulation.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/algorithms/synthesis/syrec_synthesis.hpp          
            ${${PROJECT_NAME}_SOURCE_DIR}/include/algorithms/synthesis/syrec_synthesis_additional_lines.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/algorithms/synthesis/syrec_synthesis_no_additional_lines.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/algorithms/synthesis/dd_synthesis.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/algorithms/synthesis/encoding.hpp

            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/program.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/variable.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/core/truthTable/truth_table.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/io/pla_parser.cpp
            
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/custom_semantic_errors.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/expression_evaluation_result.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/method_call_guess.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/operation.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/range_check.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/signal_evaluation_result.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/symbol_table.cpp
			
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCBaseVisitor.h
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCCustomVisitor.h
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCLexer.h
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCParser.h
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCParserErrorListener.h
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCVisitor.h

            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCBaseVisitor.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCCustomVisitor.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCLexer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCParser.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCParserErrorListener.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCVisitor.cpp

            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/gate.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/circuit.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/custom_semantic_errors.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/expression_evaluation_result.hpp
			${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/method_call_guess.hpp
			${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/operation.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/parser_config.hpp
			${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/range_check.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/signal_evaluation_result.hpp
			${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/symbol_table.hpp
			${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser/text_utils.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/expression.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/grammar.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/module.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/number.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/parser.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/program.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/statement.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/syrec/variable.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/utils/timer.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/truthTable/truth_table.hpp
            ${${PROJECT_NAME}_SOURCE_DIR}/include/core/io/pla_parser.hpp)

# set include directories
target_include_directories(${PROJECT_NAME}
                           PUBLIC $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>)

# set required C++ standard and disable compiler specific extensions
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

# add MQT::qfr library
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/qfr" "extern/qfr")
target_link_libraries(${PROJECT_NAME} PUBLIC MQT::qfr)

# add fmt library
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/fmt" "extern/fmt")
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)


# add header-only part of the Boost library
set(BOOST_USE_MULTITHREADED ON)
set(BOOST_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.71 REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)

# enable interprocedural optimization if it is supported
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
  set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# set compiler flags depending on compiler
if(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC /utf-8)
else()
  target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra $<$<CONFIG:DEBUG>:-Og>)
  if(BINDINGS)
    # adjust visibility settings for building Python bindings
    target_compile_options(${PROJECT_NAME} PUBLIC -fvisibility=hidden)
  endif()
  if(NOT DEPLOY)
    # only include machine-specific optimizations when building for the host machine
    target_compile_options(${PROJECT_NAME} PUBLIC -mtune=native)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-march=native HAS_MARCH_NATIVE)
    if(HAS_MARCH_NATIVE)
      target_compile_options(${PROJECT_NAME} PUBLIC -march=native)
    endif()
  endif()
endif()

if(GENERATE_POSITION_INDEPENDENT_CODE OR BINDINGS)
  include(CheckPIESupported)
  check_pie_supported()
  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endif()

# add coverage compiler and linker flag to the library and all targets that link against it, if
# COVERAGE is set
if(COVERAGE)
  target_compile_options(${PROJECT_NAME} PUBLIC --coverage)
  target_link_libraries(${PROJECT_NAME} PUBLIC --coverage)
endif()

# ANTLR specific instructions
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)

# using /MD flag for antlr4_runtime (for Visual C++ compilers only)
set(ANTLR4_WITH_STATIC_CRT OFF)

# Specify the version of the antlr4 library needed for this project.
# By default the latest version of antlr4 will be used.  You can specify a
# specific, stable version by setting a repository tag value or a link
# to a zip file containing the libary source.
# set(ANTLR4_TAG 4.11.1)
# set(ANTLR4_ZIP_REPOSITORY https://github.com/antlr/antlr4/archive/refs/tags/4.11.1.zip)

# add external build for antlrcpp
include(ExternalAntlr4Cpp)
include_directories(${ANTLR4_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE antlr4_static)

# add MQT alias
add_library(MQT::${PROJECT_NAME} ALIAS ${PROJECT_NAME})