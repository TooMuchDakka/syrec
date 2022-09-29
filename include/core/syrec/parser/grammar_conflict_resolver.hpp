#ifndef GRAMMAR_CONFLICT_RESOLVER_HPP
#define GRAMMAR_CONFLICT_RESOLVER_HPP

//#include "core/syrec/parser/Parser.h"
//#include "core/syrec/parser/Scanner.h"

#include <set>
#include <vector>
#include <string_view>

namespace parser {
    class Parser;
    class Scanner;
    class Token;

    [[nodiscard]] bool shouldTakeNumberInsteadOfBinaryAlternative(const Parser* parser) noexcept;
    [[nodiscard]] bool shouldTakeBinaryInsteadOfShiftAlternative(const Parser* parser) noexcept;
    [[nodiscard]] bool shouldTakeAssignInsteadOfSwapAlternative(const Parser* parser) noexcept;
    [[nodiscard]] bool checkIsLoopVariableExplicitlyDefined(const Parser* parser) noexcept;
    [[nodiscard]] bool checkIsLoopInitialValueExplicitlyDefined(const Parser* parser) noexcept;

    [[nodiscard]] bool doesTokenValueMatchAny(const Token* token, const std::vector<std::string>& possibleTokenValues) noexcept;
    [[nodiscard]] bool peekUntilTokenValuesMatchesOrEOF(const Parser* parser, const std::vector<std::string>& valuesToLookFor, const std::vector<std::string>& valuesCancellingSearch) noexcept;
    [[nodiscard]] bool isTokenSequenceNumberProduction(Scanner* scanner, const Token* token);
    [[nodiscard]] bool isTokenStartOfNonRecursiveNumber(const Token* token) noexcept;

    [[nodiscard]] std::set<std::string, std::less<>> createLookupForValues(const std::vector<std::string>& values) noexcept;

}

#endif // !GRAMMAR_CONFLICT_RESOLVER
