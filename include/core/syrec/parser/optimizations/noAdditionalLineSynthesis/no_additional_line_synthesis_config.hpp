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
        /**
         * \brief Allow certain transformations of two subsequent subtraction, subtraction and addition (or vice versa) to take place if such transformations would allow for generated of further sub-assignments, which  can lead to a lower amount of required additional lines.
         */
        bool                       transformationOfSubtractionAndAdditionOperandsForSubAssignmentCreationEnabled = false;
    };
}
#endif