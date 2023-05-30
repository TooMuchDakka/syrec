#ifndef LOOP_OPTIMIZER_HPP
#define LOOP_OPTIMIZER_HPP

#include <optional>
#include "core/syrec/statement.hpp"

namespace optimizations {
	struct LoopOptimizationConfig {
        const std::size_t maxUnrollCountPerLoop;
        const std::size_t maxAllowedNestingLevelOfInnerLoops;
        const std::size_t maxAllowedTotalLoopSize;
        const bool        allowRemainderLoop;
        const bool        forceUnrollAll;

        LoopOptimizationConfig(const std::size_t maxUnrollCountPerLoop, const std::size_t maxAllowedNestingLevelOfInnerLoops, const std::size_t maxAllowedTotalLoopSize, const bool allowRemainderLoop, const bool forceUnrollAll):
            maxUnrollCountPerLoop(maxUnrollCountPerLoop), maxAllowedNestingLevelOfInnerLoops(maxAllowedNestingLevelOfInnerLoops), maxAllowedTotalLoopSize(maxAllowedTotalLoopSize), allowRemainderLoop(allowRemainderLoop), forceUnrollAll(forceUnrollAll)
	    {}
	};

    class LoopUnroller {
    public:
        struct LoopIterationInfo {
            std::optional<std::string> loopVariableIdent;
            std::size_t initialLoopVariableValue;
            std::size_t finalLoopVariableValue;
            std::size_t stepSize;

            LoopIterationInfo(std::optional<std::string> loopVariableIdent, const std::size_t initialLoopVariableValue, const std::size_t finalLoopVariableValue, const std::size_t stepSize):
                loopVariableIdent(std::move(loopVariableIdent)), initialLoopVariableValue(initialLoopVariableValue), finalLoopVariableValue(finalLoopVariableValue), stepSize(stepSize) {}
        };

        // Forward declaration for parent struct
        struct UnrollInformation;

        struct PartiallyUnrolledLoopInformation {
            LoopIterationInfo modifiedLoopHeaderInformation;
            std::size_t       internalLoopStmtRepetitions;
            std::vector<UnrollInformation> nestedLoopUnrollInformation;
            std::optional<std::pair<LoopIterationInfo, std::vector<UnrollInformation>>> loopRemainderUnrollInformation;
        };

        struct PeeledIterationsOfLoopInformation {
            std::optional<LoopIterationInfo>                                            valueLookupForLoopVariableThatWasPeeled;
            std::vector<std::vector<UnrollInformation>>                                 nestedLoopUnrollInformationPerPeeledIteration;
            std::optional<std::pair<LoopIterationInfo, std::vector<UnrollInformation>>> loopRemainedUnrollInformation;
        };

        struct FullyUnrolledLoopInformation {
            std::optional<LoopIterationInfo>            valueLookupForLoopVariableThatWasRemovedDueToUnroll;
            std::vector<std::vector<UnrollInformation>> nestedLoopUnrollInformationPerIteration;
            std::size_t                                 repetitionPerStatement;

            FullyUnrolledLoopInformation(std::optional<LoopIterationInfo> valueLookupForLoopVariableThatWasRemovedDueToUnroll, std::vector<std::vector<UnrollInformation>> nestedLoopUnrollInformationPerIteration, std::size_t repetitionPerStatement)
                : valueLookupForLoopVariableThatWasRemovedDueToUnroll(std::move(valueLookupForLoopVariableThatWasRemovedDueToUnroll)), nestedLoopUnrollInformationPerIteration(std::move(nestedLoopUnrollInformationPerIteration)), repetitionPerStatement(repetitionPerStatement) {}
        };

        struct NotModifiedLoopInformation {
            syrec::ForStatement::ptr loopStmt;
        };

        struct UnrollInformation {
            std::variant<FullyUnrolledLoopInformation, PartiallyUnrolledLoopInformation, PeeledIterationsOfLoopInformation, NotModifiedLoopInformation> data;

            explicit UnrollInformation(std::variant<FullyUnrolledLoopInformation, PartiallyUnrolledLoopInformation, PeeledIterationsOfLoopInformation, NotModifiedLoopInformation> data)
                : data(std::move(data)) {}
        };

        // TODO: LOOP_UNROLLING: Instead of copying all entries of the existing loop variable lookup we could also simply store a reference to it 
        explicit LoopUnroller(syrec::Number::loop_variable_mapping existingLoopVariableValueLookup):
            loopVariableValueLookup(std::move(existingLoopVariableValueLookup)) {}

        UnrollInformation tryUnrollLoop(const LoopOptimizationConfig& loopUnrollConfig, const std::shared_ptr<syrec::ForStatement>& loopStatement);

    private:
        std::map<std::string, LoopIterationInfo> iterationInfoPerLoopVariable;
        syrec::Number::loop_variable_mapping     loopVariableValueLookup;

        [[nodiscard]] UnrollInformation                                                                                         tryUnrollLoop(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement);
        [[nodiscard]] std::optional<optimizations::LoopUnroller::FullyUnrolledLoopInformation>                                  canLoopBeFullyUnrolled(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement);
        [[nodiscard]] std::optional<optimizations::LoopUnroller::PeeledIterationsOfLoopInformation>                             canLoopIterationsBePeeled(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement);
        [[nodiscard]] std::optional<optimizations::LoopUnroller::PartiallyUnrolledLoopInformation>                              canLoopBePartiallyUnrolled(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement);
        [[nodiscard]] static std::optional<std::size_t>                                                                         estimateUnrolledLoopSize(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement);
        [[nodiscard]] std::optional<std::size_t>                                                                                determineNumberOfLoopIterations(const std::shared_ptr<syrec::ForStatement>& loopStatement) const;
        [[nodiscard]] static std::optional<std::string>                                                                         tryGetLoopVariable(const std::shared_ptr<syrec::ForStatement>& loopStatement);
        [[nodiscard]] LoopIterationInfo                                                                                         determineLoopIterationInfo(const std::shared_ptr<syrec::ForStatement>& loopStatement) const;
        [[nodiscard]] static LoopIterationInfo                                                                                  determineLoopHeaderInformationForNotPeeledLoopRemainder(const LoopIterationInfo& initialLoopIterationInformation, std::size_t numPeeledIterations);

        struct NestedLoopInformation {
            const std::shared_ptr<syrec::ForStatement> loopStmt;
            const std::vector<NestedLoopInformation> nestedLoops;
        };

        [[nodiscard]] static NestedLoopInformation buildLoopStructure(const std::shared_ptr<syrec::ForStatement>& loopStatement);
    };

};
#endif