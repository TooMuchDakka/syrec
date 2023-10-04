#include "core/syrec/parser/utils/message_utils.hpp"
#include <sstream>

int messageUtils::Message::Position::compare(const Position& otherPosition) const {
    if (line < otherPosition.line) {
        return -1;
    }
    if (line - otherPosition.line) {
        return 1;
    }

    if (column < otherPosition.column) {
        return -1;
    }
    return static_cast<int>(column - otherPosition.column);
}


messageUtils::Message messageUtils::MessageFactory::createMessage(const Message::ErrorCategory errorCategory, const Message::Position position, const Message::Severity severity, const std::string_view& message) {
    return Message({errorCategory, position, severity, std::string(message)});
}

messageUtils::Message messageUtils::MessageFactory::createError(const Message::ErrorCategory errorCategory, Message::Position position, const std::string_view& message) {
    return createMessage(errorCategory, position, Message::Severity::Error, message);
}


std::optional<std::string> messageUtils::tryStringifyMessage(const Message& message, const std::string_view& messageFormat) {
    if (messageFormat != Message::defaultStringifiedMessageFormat) {
        return std::nullopt;
    }
    return std::make_optional(fmt::format(Message::defaultStringifiedMessageFormat, message.position.line, message.position.column, message.text));
}

std::optional<std::string> messageUtils::tryStringifyMessages(const std::vector<Message>& messages, char const* messageDelimiter) {
    if (messages.empty()) {
        return std::nullopt;
    }

    if (messages.size() == 1) {
        return tryStringifyMessage(messages.front());
    }

    std::ostringstream containerForConcatinatedErrors;
    
    auto       stringificationOk                     = true;
    const auto idxOfLastMessageWithTrailingDelimiter = messages.size() - 2;
    for (std::size_t i = 0; i <= idxOfLastMessageWithTrailingDelimiter && stringificationOk; ++i) {
        if (const auto& stringifiedMsg = tryStringifyMessage(messages.at(i)); stringifiedMsg.has_value()) {
            containerForConcatinatedErrors << *stringifiedMsg << messageDelimiter;
            continue;
        }
        stringificationOk = false;
    }

    if (stringificationOk) {
        if (const auto& stringificationResultOfMessageWithoutTrailingDelimiter = tryStringifyMessage(messages.back()); stringificationResultOfMessageWithoutTrailingDelimiter.has_value()) {
            containerForConcatinatedErrors << *stringificationResultOfMessageWithoutTrailingDelimiter;
        } else {
            stringificationOk = false;
        }
    }

    return stringificationOk ? std::make_optional(containerForConcatinatedErrors.str()) : std::nullopt;
}

std::vector<std::string> messageUtils::tryDeserializeStringifiedMessagesFromString(const std::string_view& stringifiedMessages, char const* expectedMessageDelimiter) {
    if (stringifiedMessages.empty()) {
        return {};
    }

    std::vector<std::string> actualErrors{};
    size_t                   previousErrorEndPosition = 0;
    size_t                   currErrorEndPosition     = stringifiedMessages.find_first_of(expectedMessageDelimiter, previousErrorEndPosition);
    bool                     foundErrorSeparator;
    do {
        foundErrorSeparator = std::string::npos != currErrorEndPosition;
        if (foundErrorSeparator) {
            actualErrors.emplace_back(stringifiedMessages.substr(previousErrorEndPosition, (currErrorEndPosition - previousErrorEndPosition)));
            previousErrorEndPosition = currErrorEndPosition + 1;
            currErrorEndPosition     = stringifiedMessages.find_first_of(expectedMessageDelimiter, previousErrorEndPosition);
        } else {
            currErrorEndPosition = stringifiedMessages.size();
            actualErrors.emplace_back(stringifiedMessages.substr(previousErrorEndPosition, (currErrorEndPosition - previousErrorEndPosition)));
        }
    } while (foundErrorSeparator);
    return actualErrors;
}
