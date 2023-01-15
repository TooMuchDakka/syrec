#ifndef BINARY_EXPRESSION_SIGNAL_PROHIBIPTION_HPP
#define BINARY_EXPRESSION_SIGNAL_PROHIBIPTION_HPP
#include "core/syrec/variable.hpp"

namespace parser {
class BinaryExpressionSignalProhibition {
public:
    void reset();
    void setProhibitedSignal(const syrec::VariableAccess::ptr& prohibitedSignalAccess);
    bool isProhibitedSignalAccess(const std::string_view& signalIdent) const;
    bool isProhibitedDimensionAccess(size_t accessedDimension, size_t valueForAccessedDimension) const;
    bool isProhibitedBitAccess(size_t accessedBit) const;
    bool isProhibitedBitrangeAccess(std::pair<size_t, size_t> accessedBitRange) const;

private:
    std::optional<syrec::VariableAccess::ptr> prohibitedVariableAccess;
};
}

#endif