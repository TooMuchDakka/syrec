# add MQT::Core target
set(BUILD_MQT_CORE_TESTS
    OFF
    CACHE BOOL "Build MQT Core tests")
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/mqt-core" "extern/mqt-core" EXCLUDE_FROM_ALL)
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/fmt" "extern/fmt" EXCLUDE_FROM_ALL)

add_library( 
      ${PROJECT_NAME}
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/syrec_synthesis.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/syrec_cost_aware_synthesis.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/syrec_line_aware_synthesis.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/simulation/simple_simulation.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/simulation/circuit_to_truthtable.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/dd_synthesis.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/synthesis/encoding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/optimization/esop_minimization.cpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/optimization/esop_minimization.hpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/simulation/simple_simulation.hpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/simulation/circuit_to_truthtable.hpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/synthesis/syrec_synthesis.hpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/synthesis/syrec_cost_aware_synthesis.hpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/synthesis/syrec_line_aware_synthesis.hpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/synthesis/dd_synthesis.hpp
      ${PROJECT_SOURCE_DIR}/include/algorithms/synthesis/encoding.hpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/program.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/variable.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/truthTable/truth_table.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/io/pla_parser.cpp
      
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/custom_semantic_errors.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/expression_comparer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/expression_evaluation_result.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/module_call_guess.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/module_call_stack.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/operation.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/range_check.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/signal_access_restriction.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/signal_evaluation_result.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/symbol_table.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/symbol_table_backup_helper.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/value_broadcaster.cpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCBaseVisitor.h
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCLexer.h
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCParser.h
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCErrorListener.h
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCVisitor.h

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCBaseVisitor.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCLexer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCParser.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCErrorListener.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/SyReCVisitor.cpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/parserComponents/syrec_expression_visitor.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/parserComponents/syrec_module_visitor.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/antlr/parserComponents/syrec_statement_visitor.cpp
      
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/combine_redundant_instructions.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/dead_code_elimination.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/loop_optimizer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/no_additional_line_assignment.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/operation_strength_reduction.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/optimizer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/reassociate_expression.cpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.cpp

      #${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.cpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/deadStoreElimination/dead_store_eliminator.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/deadStoreElimination/dead_store_status_lookup.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/deadStoreElimination/statement_iteration_helper.cpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_transformer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_none_reversible_operations_and_unique_signal_occurrences_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_only_reversible_operations_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_reversible_ops_and_multilevel_signal_occurrence_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_without_additional_lines_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/base_assignment_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_substitution_generator.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_to_subassignment_splitter.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/inorder_operation_node_traversal_helper.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_expressions_container.cpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/operationSimplification/base_multiplication_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/operationSimplification/binary_factoring_multiplication_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/optimizations/operationSimplification/binary_multiplication_simplifier.cpp

      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/bit_helpers.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/binary_expression_simplifier.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/copy_utils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/dangerous_component_check_utils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/loop_body_value_propagation_blocker.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/loop_range_utils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/message_utils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/signal_access_utils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syrec/parser/utils/syrec_ast_dump_utils.cpp

      ${PROJECT_SOURCE_DIR}/include/core/gate.hpp
      ${PROJECT_SOURCE_DIR}/include/core/circuit.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/expression.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/grammar.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/module.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/number.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/program.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/statement.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/variable.hpp
      ${PROJECT_SOURCE_DIR}/include/core/utils/timer.hpp
      ${PROJECT_SOURCE_DIR}/include/core/truthTable/truth_table.hpp
      ${PROJECT_SOURCE_DIR}/include/core/io/pla_parser.hpp

      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/custom_semantic_errors.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/expression_comparer.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/expression_evaluation_result.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/module_call_guess.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/module_call_stack.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/operation.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/parser_config.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/range_check.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/signal_access_restriction.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/signal_evaluation_result.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/symbol_table.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/symbol_table_backup_helper.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/value_broadcaster.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor_shared_data.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/antlr/parserComponents/syrec_expression_visitor.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/antlr/parserComponents/syrec_module_visitor.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/antlr/parserComponents/syrec_parser_interface.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/antlr/parserComponents/syrec_statement_visitor.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/combine_redundant_instructions.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/dead_code_elimination.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/loop_optimizer.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/no_additional_line_assignment.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/operation_strength_reduction.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/optimizer.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/reassociate_expression.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/constantPropagation/shared_dimension_blocking_information.hpp

      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.tpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/constantPropagation/valueLookup/layer_data.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp

      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/deadStoreElimination/dead_store_eliminator.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/deadStoreElimination/dead_store_status_lookup.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/deadStoreElimination/statement_iteration_helper.hpp

      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_transformer.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_none_reversible_operations_and_unique_signal_occurrences_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_only_reversible_operations_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_reversible_ops_and_multilevel_signal_occurrence_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_without_additional_lines_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/base_assignment_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_substitution_generator.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_to_subassignment_splitter.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/inorder_operation_node_traversal_helper.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/no_additional_line_synthesis_config.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_expressions_container.hpp

      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/operationSimplification/base_multiplication_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/operationSimplification/binary_factoring_multiplication_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/optimizations/operationSimplification/binary_multiplication_simplifier.hpp

      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/bit_helpers.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/binary_expression_simplifier.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/copy_utils.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/dangerous_component_check_utils.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/loop_body_value_propagation_blocker.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/loop_range_utils.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/message_utils.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/signal_access_utils.hpp
      ${PROJECT_SOURCE_DIR}/include/core/syrec/parser/utils/syrec_ast_dump_utils.hpp)

# set include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                  ${PROJECT_BINARY_DIR}/include)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src/core/syrec/parser/antlr)

# link to the MQT::qfr DD library
target_link_libraries(${PROJECT_NAME} PUBLIC MQT::CoreDD)

# add fmt library
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)

# add header-only part of the Boost library
set(BOOST_USE_MULTITHREADED ON)
set(BOOST_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.71 REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)

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

target_link_libraries(${PROJECT_NAME} PUBLIC antlr4_static)

# add MQT alias
add_library(MQT::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

if(BUILD_MQT_SYREC_BINDINGS)
  add_subdirectory(python)
endif()