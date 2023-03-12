#ifndef NO_ADDITIONAL_LINE_ASSIGNMENT_HPP
#define NO_ADDITIONAL_LINE_ASSIGNMENT_HPP
#pragma once

#include "core/syrec/statement.hpp"

#include <optional>

namespace optimizations {
	struct LineAwareOptimizationResult {
        std::optional<syrec::AssignStatement::vec> createdStatements;
	};

	[[nodiscard]] static LineAwareOptimizationResult optimize(const syrec::AssignStatement& assignmentStatement);
};
#endif