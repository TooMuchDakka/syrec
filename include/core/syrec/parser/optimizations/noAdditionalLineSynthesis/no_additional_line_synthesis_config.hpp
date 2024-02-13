#ifndef NO_ADDITIONAL_LINE_SYNTHESIS_CONFIG_HPP
#define NO_ADDITIONAL_LINE_SYNTHESIS_CONFIG_HPP
#pragma once

namespace noAdditionalLineSynthesis {
    struct NoAdditionalLineSynthesisConfig {
        bool useGeneratedAssignmentsByDecisionAsTieBreaker = false;
        /**
         * \brief Prefer the simplification result of the more complex algorithm regardless of the costs in comparision to the simplification result of the "simpler" algorithm. If the former did not produce any assignments, the results from the latter are used instead.
         */
        bool preferAssignmentsGeneratedByChoiceRegardlessOfCost = false;
        /**
         * \brief This additional factor for the cost of an expression can be used to achieve a balance between code-size and the number of additional signals required for the synthesis of an assignment
         */
        double                     expressionNestingPenalty                       = 75;
        double                     expressionNestingPenaltyScalingPerNestingLevel = 3.5;
        std::optional<std::string> optionalNewReplacementSignalIdentsPrefix;
    };
}
#endif