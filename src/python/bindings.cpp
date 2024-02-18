#include "algorithms/simulation/simple_simulation.hpp"
#include "algorithms/synthesis/syrec_cost_aware_synthesis.hpp"
#include "algorithms/synthesis/syrec_line_aware_synthesis.hpp"
#include "core/circuit.hpp"
#include "core/gate.hpp"
#include "core/properties.hpp"
#include "core/syrec/program.hpp"

#include <boost/dynamic_bitset.hpp>
#include <functional>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;
using namespace syrec;

PYBIND11_MODULE(pysyrec, m) {
    m.doc() = "Python interface for the SyReC programming language for the synthesis of reversible circuits";

    py::class_<Circuit, std::shared_ptr<Circuit>>(m, "circuit")
            .def(py::init<>(), "Constructs circuit object.")
            .def_property("lines", &syrec::Circuit::getLines, &syrec::Circuit::setLines, "Returns the number of circuit lines.")
            .def_property_readonly("num_gates", &syrec::Circuit::numGates, "Returns the total number of gates in the circuit.")
            .def("__iter__",
                 [](Circuit& circ) { return py::make_iterator(circ.begin(), circ.end()); })
            .def_property("inputs", &syrec::Circuit::getInputs, &syrec::Circuit::setInputs, "Returns the input names of the lines in a circuit.")
            .def_property("outputs", &syrec::Circuit::getOutputs, &syrec::Circuit::setOutputs, "Returns the output names of the lines in a circuit.")
            .def_property("constants", &syrec::Circuit::getConstants, &syrec::Circuit::setConstants, "Returns the constant input line specification.")
            .def_property("garbage", &syrec::Circuit::getGarbage, &syrec::Circuit::setGarbage, "Returns whether outputs are garbage or not.")
            .def(
                    "annotations", [](const Circuit& c, const Gate& g) {
                        py::dict   d{};
                        const auto annotations = c.getAnnotations(g);
                        if (annotations) {
                            for (const auto& [first, second]: *annotations) {
                                d[py::cast(first)] = second;
                            }
                        }
                        return d;
                    },
                    "This method returns all annotations for a given gate.")
            .def("quantum_cost", &syrec::Circuit::quantumCost, "Returns the quantum cost of the circuit.")
            .def("transistor_cost", &syrec::Circuit::transistorCost, "Returns the transistor cost of the circuit.")
            .def("to_qasm_str", &syrec::Circuit::toQasm, "Returns the QASM representation of the circuit.")
            .def("to_qasm_file", &syrec::Circuit::toQasmFile, "filename"_a, "Writes the QASM representation of the circuit to a file.");

    py::class_<Properties, std::shared_ptr<Properties>>(m, "properties")
            .def(py::init<>(), "Constructs property map object.")
            .def("set_string", &Properties::set<std::string>)
            .def("set_bool", &Properties::set<bool>)
            .def("set_int", &Properties::set<int>)
            .def("set_unsigned", &Properties::set<unsigned>)
            .def("set_double", &Properties::set<double>)
            .def("get_string", py::overload_cast<const std::string&>(&Properties::get<std::string>, py::const_))
            .def("get_double", py::overload_cast<const std::string&>(&Properties::get<double>, py::const_));

    /*
     * We are following the following naming convention (https://peps.python.org/pep-0008/#function-and-variable-names) for all variables names of the defined classes
     */
    py::class_<optimizations::LoopOptimizationConfig>(m, "loop_optimization_config")
            .def(py::init(), "Constructs a loop optimization config object.")
            .def_readwrite("max_number_of_unrolled_iterations_per_loop", &optimizations::LoopOptimizationConfig::maxNumberOfUnrolledIterationsPerLoop)
            .def_readwrite("max_allowed_nesting_level_of_inner_loops", &optimizations::LoopOptimizationConfig::maxAllowedNestingLevelOfInnerLoops)
            .def_readwrite("max_allowed_loop_size_in_number_of_statements", &optimizations::LoopOptimizationConfig::maxAllowedLoopSizeInNumberOfStatements)
            .def_readwrite("is_remainder_loop_allowed", &optimizations::LoopOptimizationConfig::isRemainderLoopAllowed)
            .def_readwrite("force_unroll", &optimizations::LoopOptimizationConfig::forceUnroll)
            .def_readwrite("auto_unroll_loops_with_one_iteration", &optimizations::LoopOptimizationConfig::autoUnrollLoopsWithOneIteration);

    py::class_<noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig>(m, "no_additional_line_synthesis_config")
            .def(py::init(), "Constructs a additional line synthesis config object.")
            .def_readwrite("use_generated_assignments_by_decisions_as_tie_breaker", &noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig::useGeneratedAssignmentsByDecisionAsTieBreaker)
            .def_readwrite("prefer_assignments_generated_by_choice_regardless_of_cost", &noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig::preferAssignmentsGeneratedByChoiceRegardlessOfCost)
            .def_readwrite("expression_nesting_penalty", &noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig::expressionNestingPenalty)
            .def_readwrite("expression_nesting_penalty_scaling_per_nesting_level", &noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig::expressionNestingPenaltyScalingPerNestingLevel)
            .def_readwrite("optional_new_replacement_signal_idents_prefix", &noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig::optionalNewReplacementSignalIdentsPrefix);

    /*
     * We are following the official naming conventions (https://docs.python.org/3/howto/enum.html) recommending the following naming schema for enum members (UPPER_CASE).
     * (see also: https://peps.python.org/pep-0008/#constants)
     */
     py::enum_<optimizations::MultiplicationSimplificationMethod>(m, "MultiplicationSimplificationMethod")
            .value("BINARY_SIMPLIFICATION", optimizations::MultiplicationSimplificationMethod::BinarySimplification, "Simplify multiplications using the binary simplification method.")
            .value("BINARY_SIMPLIFICATION_WITH_FACTORING", optimizations::MultiplicationSimplificationMethod::BinarySimplificationWithFactoring, "Simplify multiplications using the binary simplification method.")
            .value("NONE", optimizations::MultiplicationSimplificationMethod::None, "None.")
            .export_values();

    py::class_<ReadProgramSettings>(m, "read_program_settings")
            .def(py::init(), "Constructs a ReadProgramSettings object.")
            .def_readwrite("default_bitwidth", &ReadProgramSettings::defaultBitwidth)
            .def_readwrite("reassociate_expression_enabled", &ReadProgramSettings::reassociateExpressionEnabled)
            .def_readwrite("dead_code_elimination_enabled", &ReadProgramSettings::deadCodeEliminationEnabled)
            .def_readwrite("constant_propagation_enabled", &ReadProgramSettings::constantPropagationEnabled)
            .def_readwrite("operation_strength_reduction_enabled", &ReadProgramSettings::operationStrengthReductionEnabled)
            .def_readwrite("dead_store_elimination_enabled", &ReadProgramSettings::deadStoreEliminationEnabled)
            .def_readwrite("combine_redundant_instructions", &ReadProgramSettings::combineRedundantInstructions)
            .def_readwrite("multiplication_simplification_method", &ReadProgramSettings::multiplicationSimplificationMethod)
            .def_readwrite("optional_loop_optimization_config", &ReadProgramSettings::optionalLoopUnrollConfig)
            .def_readwrite("optional_no_additional_line_synthesis_config", &ReadProgramSettings::optionalNoAdditionalLineSynthesisConfig)
            .def_readwrite("expected_main_module_name", &ReadProgramSettings::expectedMainModuleName);

    py::class_<Program::OptimizationResult>(m, "optimization_result")
            .def(py::init(), "Constructs the optimization result of a SyReC program.")
            .def_readwrite("found_errors", &Program::OptimizationResult::foundErrors)
            .def_readwrite("dumped_optimized_circuit", &Program::OptimizationResult::dumpedOptimizedCircuit);

    py::class_<Program>(m, "program")
            .def(py::init<>(), "Constructs SyReC program object.")
            .def("add_module", &Program::addModule)
            .def("read", &Program::read, "filename"_a, "settings"_a = ReadProgramSettings{}, "Read a SyReC program from a file.")
            .def("read_and_optimize_from_file", &Program::readAndOptimizeCircuit, "circuitFileName"_a, "settings"_a = ReadProgramSettings{}, "Read and optimize a SyReC program from a file.")
            .def("read_and_optimize_from_string", &Program::readAndOptimizeCircuitFromString, "circuitStringified"_a, "settings"_a = ReadProgramSettings{}, "Read and optimize a SyReC program from a string.");

    py::class_<boost::dynamic_bitset<>>(m, "bitset")
            .def(py::init<>(), "Constructs bitset object of size zero.")
            .def(py::init<int>(), "n"_a, "Constructs bitset object of size n.")
            .def(py::init<int, uint64_t>(), "n"_a, "val"_a, "Constructs a bitset object of size n from an integer val")
            .def("set", py::overload_cast<boost::dynamic_bitset<>::size_type, bool>(&boost::dynamic_bitset<>::set), "n"_a, "val"_a, "Sets bit n if val is true, and clears bit n if val is false")
            .def(
                    "__str__", [](const boost::dynamic_bitset<>& b) {
                        std::string str{};
                        boost::to_string(b, str);
                        std::reverse(str.begin(), str.end());
                        return str;
                    },
                    "Returns the equivalent string of the bitset.");

    py::enum_<Gate::Types>(m, "gate_type")
            .value("toffoli", Gate::Types::Toffoli, "Toffoli gate type.")
            .value("fredkin", Gate::Types::Fredkin, "Fredkin gate type.")
            .export_values();

    py::class_<Gate, std::shared_ptr<Gate>>(m, "gate")
            .def(py::init<>(), "Constructs gate object.")
            .def_readwrite("controls", &Gate::controls, "Controls of the gate.")
            .def_readwrite("targets", &Gate::targets, "Targets of the gate.")
            .def_readwrite("type", &Gate::type, "Type of the gate.");

    m.def("cost_aware_synthesis", &CostAwareSynthesis::synthesize, "circ"_a, "program"_a, "settings"_a = Properties::ptr(), "statistics"_a = Properties::ptr(), "Cost-aware synthesis of the SyReC program.");
    m.def("line_aware_synthesis", &LineAwareSynthesis::synthesize, "circ"_a, "program"_a, "settings"_a = Properties::ptr(), "statistics"_a = Properties::ptr(), "Line-aware synthesis of the SyReC program.");
    m.def("simple_simulation", &simpleSimulation, "output"_a, "circ"_a, "input"_a, "statistics"_a = Properties::ptr(), "Simulation of the synthesized circuit circ.");
}