#ifndef LOOP_OPTIMIZER_HPP
#define LOOP_OPTIMIZER_HPP
#pragma once

#include <optional>
#include "core/syrec/statement.hpp"

namespace optimizations {
	struct LoopOptimizationConfig {
        std::size_t maxNumberOfUnrolledIterationsPerLoop;
        std::size_t maxAllowedNestingLevelOfInnerLoops;
        std::size_t maxAllowedLoopSizeInNumberOfStatements;
        bool        isRemainderLoopAllowed;
        bool        forceUnroll;
        bool        autoUnrollLoopsWithOneIteration;
	};

	class LoopUnroller {
	public:
        LoopUnroller()
	        : unrollConfig(LoopOptimizationConfig()) {}

        virtual ~LoopUnroller() = default;

		struct LoopHeaderInformation {
            std::optional<std::string>  loopVariableIdent;
            unsigned int                initialLoopVariableValue;
            unsigned int                finalLoopVariableValue;
            unsigned int                stepSize;
		};
        
		struct BaseUnrolledLoopInformation {
            std::size_t      numUnrolledIterations;

            BaseUnrolledLoopInformation()
		        : numUnrolledIterations(0) {}

            virtual ~BaseUnrolledLoopInformation() = default;
		};

		struct UnmodifiedLoopInformation : BaseUnrolledLoopInformation {
		};

		struct LoopVariableValueInformation {
            std::string loopVariableIdent;
            std::vector<unsigned int> valuePerIteration;
		};

		struct UnrolledIterationInformation {
            std::vector<std::unique_ptr<syrec::Statement>> iterationStatements;
            std::optional<LoopVariableValueInformation>    loopVariableValuePerIteration;
		};

		struct PartiallyUnrolledLoopInformation : BaseUnrolledLoopInformation {
            std::vector<std::unique_ptr<syrec::Statement>> peeledStatements;
            UnrolledIterationInformation                   unrolledIterations;
            LoopHeaderInformation                          remainderLoopHeaderInformation;
            std::vector<std::unique_ptr<syrec::Statement>> remainderLoopBodyStatements;
		};

		struct FullyUnrolledLoopInformation : BaseUnrolledLoopInformation {
            std::vector<std::unique_ptr<syrec::Statement>> peeledStatements;
            UnrolledIterationInformation                   unrolledIterations;
		};

		[[nodiscard]] std::unique_ptr<BaseUnrolledLoopInformation> tryUnrollLoop(std::size_t currentLoopNestingLevel, const syrec::ForStatement& loopStatement, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings, const LoopOptimizationConfig& unrollConfigToUse);
	protected:
        LoopOptimizationConfig unrollConfig;
        syrec::Number::loop_variable_mapping knownLoopVariableValueMappings;
        
        virtual void                                                         resetInternals(const LoopOptimizationConfig& newUnrollConfig, const syrec::Number::loop_variable_mapping& newKnownLoopVariableValueMappings);
        [[nodiscard]] bool                                                   canLoopBeFullyUnrolled(const LoopHeaderInformation& loopHeaderInformation) const;
        [[nodiscard]] bool                                                   doesCurrentLoopNestingLevelAllowUnroll(std::size_t currentLoopNestingLevel) const;
        [[nodiscard]] static std::optional<FullyUnrolledLoopInformation>     tryPerformFullLoopUnroll(const LoopHeaderInformation& loopHeader, const syrec::Statement::vec& loopBodyStatements);
        [[nodiscard]] static std::optional<PartiallyUnrolledLoopInformation> tryPerformPartialUnrollOfLoop(std::size_t maxNumberOfUnrolledIterationsPerLoop, bool isLoopRemainderAllowed, const LoopHeaderInformation& loopHeader, const syrec::Statement::vec& loopBodyStatements);
        [[nodiscard]] static std::optional<unsigned int>                     tryEvaluateNumberToConstant(const syrec::Number& numberContainer, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings);
        [[nodiscard]] static std::optional<unsigned int>                     tryEvaluateCompileTimeConstantExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings);
        [[nodiscard]] static std::optional<LoopHeaderInformation>            tryDetermineIterationRangeForLoopHeader(const syrec::ForStatement& loopStatement, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings);
        [[nodiscard]] static std::optional<unsigned int>                     tryDetermineNumberOfLoopIterations(const LoopHeaderInformation& loopHeader);
        [[nodiscard]] static std::unique_ptr<syrec::ForStatement>            createLoopStatements(const LoopHeaderInformation& loopHeader, const std::vector<std::reference_wrapper<const syrec::Statement>>& loopBody);
	};
};
#endif