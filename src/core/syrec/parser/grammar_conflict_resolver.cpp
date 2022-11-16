#include "core/syrec/parser/grammar_conflict_resolver.hpp"
#include "core/syrec/parser/Parser.h"

using namespace parser;

bool parser::shouldTakeNumberInsteadOfBinaryAlternative(const Parser* parser) noexcept {
    return parser::isTokenSequenceNumberProduction(parser->scanner, parser->la);
}

bool parser::shouldTakeBinaryInsteadOfShiftAlternative(const Parser* parser) noexcept {
    // Since first LL(1) conflict resolver already started peeking and failed to resolve the conflict, we need to reset the peek position to original one
    parser->scanner->ResetPeek();

    const std::set<std::string, std::less<>> binaryAlternativeTokenValues = parser::createLookupForValues({"+", "-", "^", "*", "/", "%", "*>", "&&", "||", "&", "|", "<", ">", "=", "!=", "<=", ">="});
    const std::set<std::string, std::less<>> shiftAlternativeTokenValues  = parser::createLookupForValues({"<<", ">>"});

    bool canCancelSearch = false;
    bool foundMatchingTokenValue = false;
    size_t expressionNestingLevel = 0;

    std::shared_ptr<Token> peekedToken = parser->scanner->Peek();
    
    while (!canCancelSearch && peekedToken->kind != 0) {
        const std::string tokenValue = peekedToken->val;
        if (tokenValue == "(") {
            expressionNestingLevel++;
        }
        else if (tokenValue == ")") {
            canCancelSearch = expressionNestingLevel == 0;
            expressionNestingLevel--;
        } else {
            foundMatchingTokenValue = expressionNestingLevel == 0 && binaryAlternativeTokenValues.count(tokenValue);
            canCancelSearch         = foundMatchingTokenValue || shiftAlternativeTokenValues.count(tokenValue);
        }
        peekedToken = parser->scanner->Peek();
    }
    return foundMatchingTokenValue;
}

bool parser::shouldTakeAssignInsteadOfSwapAlternative(const Parser* parser) noexcept {
    parser->scanner->ResetPeek();
    return parser::peekUntilTokenValuesMatchesOrEOF(parser, {"^", "+", "-"}, {";", "<=>"});
}

bool parser::checkIsLoopVariableExplicitlyDefined(const Parser* parser) noexcept {
    // Since first LL(1) conflict resolver already started peeking and failed to resolve the conflict, we need to reset the peek position to original one
    parser->scanner->ResetPeek();
    return parser::peekUntilTokenValuesMatchesOrEOF(parser, {"="},{"rof"});
}

bool parser::checkIsLoopInitialValueExplicitlyDefined(const Parser* parser) noexcept {
    return parser::peekUntilTokenValuesMatchesOrEOF(parser, {"to"}, {"rof"});
}

bool parser::doesTokenValueMatchAny(const std::shared_ptr<Token>& token, const std::vector<std::string>& possibleTokenValues) noexcept {
    if (nullptr == token) {
        return false;
    }

    const std::string tokenValue = token->val;
    return std::find_if(
        cbegin(possibleTokenValues),
        cend(possibleTokenValues),
        [tokenValue](const std::string_view& possibleTokenValue) { return possibleTokenValue == tokenValue; }) != cend(possibleTokenValues);
}

bool parser::peekUntilTokenValuesMatchesOrEOF(const Parser* parser,
    const std::vector<std::string>& valuesToLookFor,
    const std::vector<std::string>& valuesCancellingSearch) noexcept {

    if (valuesToLookFor.empty()) {
        return false;
    }

    bool          foundMatchingValue = false;
    bool          canCancelSearch       = false;

    const std::set<std::string, std::less<>> valueLookup             = parser::createLookupForValues(valuesToLookFor);
    const std::set<std::string, std::less<>> alternativeValuesLookup = parser::createLookupForValues(valuesCancellingSearch);

    std::shared_ptr<Token> peekedToken = parser->scanner->Peek();
    while (!canCancelSearch && peekedToken->kind != 0) {
        foundMatchingValue = valueLookup.count(peekedToken->val);
        canCancelSearch                     = foundMatchingValue || peekedToken->kind == 0 || alternativeValuesLookup.count(peekedToken->val);
        peekedToken = !canCancelSearch ? parser->scanner->Peek() : nullptr;
    }
    return foundMatchingValue;
}

bool parser::isTokenSequenceNumberProduction(const std::shared_ptr<Scanner>& scanner, const std::shared_ptr<Token>& token) {
    if (nullptr == token || 0 == token->kind) {
        return false;
    }
    
    if ("(" == token->val) {
        const std::shared_ptr<Token> peekedToken = scanner->Peek();
        return parser::isTokenSequenceNumberProduction(scanner, peekedToken)
            && parser::doesTokenValueMatchAny(scanner->Peek(), {"^", "+", "-", "*"})
            && parser::isTokenSequenceNumberProduction(scanner, scanner->Peek())
            && parser::doesTokenValueMatchAny(scanner->Peek(), {")"});
    }
    return parser::isTokenStartOfNonRecursiveNumber(token);
}

bool parser::isTokenStartOfNonRecursiveNumber(const std::shared_ptr<Token>& token) noexcept {
    return token != nullptr && (token->kind == 2 || token->val == "#" || token->val == "$");
}

std::set<std::string, std::less<>> parser::createLookupForValues(const std::vector<std::string>& values) noexcept {
    return { std::begin(values), std::end(values) };
}