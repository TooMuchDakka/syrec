if(NOT TARGET gtest OR NOT TARGET gmock)
  # Prevent overriding the parent project's compiler/linker settings on Windows
  set(gtest_force_shared_crt # cmake-lint: disable=C0103
      ON
      CACHE BOOL "" FORCE)
  add_subdirectory("${PROJECT_SOURCE_DIR}/extern/mqt-core/extern/googletest"
                   "extern/mqt-core/extern/googletest" EXCLUDE_FROM_ALL)
  set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES FOLDER extern)
endif()

add_library(${PROJECT_NAME}_test_utils INTERFACE)
target_include_directories(${PROJECT_NAME}_test_utils 
    INTERFACE ${CMAKE_SOURCE_DIR}/test/unittests/syrecComponentsTestsUtils/
)

 # create an executable in which the tests will be stored
add_executable(${PROJECT_NAME}_test 
    unittests/test_parser.cpp
    unittests/test_line_aware_synthesis.cpp
    unittests/test_line_aware_simulation.cpp
    unittests/test_cost_aware_synthesis.cpp
    unittests/test_cost_aware_simulation.cpp
    unittests/test_tt_to_dd.cpp
    unittests/test_extend_tt.cpp
    unittests/test_huffman.cpp
    unittests/test_pla_parser.cpp

    unittests/syrecOptimizerComponentsTests/test_constant_propagation_optimization.cpp
    unittests/syrecOptimizerComponentsTests/test_combine_redundant_instructions.cpp
    unittests/syrecOptimizerComponentsTests/test_dead_code_elimination.cpp
    unittests/syrecOptimizerComponentsTests/test_dead_store_elimination.cpp
    unittests/syrecOptimizerComponentsTests/test_multiplication_simplification.cpp
    unittests/syrecOptimizerComponentsTests/test_no_additional_line_synthesis.cpp
    unittests/syrecOptimizerComponentsTests/test_operation_strength_reduction.cpp
    unittests/syrecOptimizerComponentsTests/test_reassociate_expression_optimization.cpp

    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_lift_restriction_by_assignment.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_restriction_for_complete_signal.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_restriction_for_global_bitrange.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_restriction_for_value_of_dimension.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_restriction_for_whole_dimension.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_swap.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_swap.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_update_from_variable.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp
    unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup_update_value.cpp unittests/syrecOptimizerComponentsTests/signalValueLookupTests/test_signal_value_lookup.hpp

    unittests/syrecParserComponentsTests/fixtures/syrec_parser_error_cases_fixture.cpp
    unittests/syrecParserComponentsTests/fixtures/syrec_parser_method_call_guess_fixture.cpp
    unittests/syrecParserComponentsTests/fixtures/syrec_parser_range_check_fixture.cpp
    unittests/syrecParserComponentsTests/fixtures/syrec_parser_success_cases_fixture.cpp
    unittests/syrecParserComponentsTests/fixtures/syrec_parser_symbol_table_fixture.cpp
    unittests/syrecParserComponentsTests/fixtures/syrec_parser_signal_access_restriction_error_fixture.cpp unittests/syrecParserComponentsTests/fixtures/syrec_signal_operand_builder.hpp unittests/syrecParserComponentsTests/fixtures/syrec_signal_operand_builder_utilities.hpp unittests/syrecParserComponentsTests/fixtures/syrec_parser_signal_access_restriction_error_fixture.hpp
    unittests/syrecParserComponentsTests/fixtures/syrec_parser_signal_access_restriction_fixture.cpp unittests/syrecParserComponentsTests/fixtures/syrec_parser_signal_access_restriction_fixture.hpp
    unittests/syrecParserComponentsTests/fixtures/syrec_parser_signal_access_restriction_success_fixture.cpp unittests/syrecParserComponentsTests/fixtures/syrec_signal_operand_builder.hpp  unittests/syrecParserComponentsTests/fixtures/syrec_parser_signal_access_restriction_success_fixture.hpp
    unittests/test_dd_synthesis.cpp
    unittests/test_dd_synthesis_dc.cpp
)
# link the Google test infrastructure and a default main function to the test executable.
target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME} ${PROJECT_NAME}_test_utils gmock gtest_main)
# discover tests
#gtest_discover_tests(
#    ${PROJECT_NAME}_test
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
gtest_discover_tests(${PROJECT_NAME}_test)
set_target_properties(${PROJECT_NAME}_test PROPERTIES FOLDER tests)

add_custom_command(
  TARGET ${PROJECT_NAME}_test
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/${PROJECT_NAME}_test
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_test
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/circuits
          $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/circuits
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/circuits
          ${CMAKE_BINARY_DIR}/circuits
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs
          $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/configs
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/configs
          ${CMAKE_BINARY_DIR}/configs

  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/unittests
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/unittests/syrecParserComponentsTests
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/unittests/syrecOptimizerComponentsTests

  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/unittests
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/unittests/syrecParserComponentsTests
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/unittests/syrecOptimizerComponentsTests

  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/unittests/syrecParserComponentsTests/testdata
          $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/unittests/syrecParserComponentsTests/testdata
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/unittests/syrecParserComponentsTests/testdata
          ${CMAKE_BINARY_DIR}/unittests/syrecParserComponentsTests/testdata
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/unittests/syrecOptimizerComponentsTests/testdata
          $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/unittests/syrecOptimizerComponentsTests/testdata
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_DIR:${PROJECT_NAME}_test>/unittests/syrecOptimizerComponentsTests/testdata
          ${CMAKE_BINARY_DIR}/unittests/syrecOptimizerComponentsTests/testdata

  COMMENT "Copying circuits and creating symlinks for ${PROJECT_NAME}_test"
  VERBATIM)