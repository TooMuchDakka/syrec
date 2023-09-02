#include "core/syrec/parser/utils/message_utils.hpp"
#include "core/syrec/parser/infix_iterator.hpp"

#include <sstream>

messageUtils::Message messageUtils::MessageFactory::createMessage(const Message::Position position, const Message::Severity severity, const std::string& message) {
    return Message({position, severity, message});
}

std::optional<std::string> messageUtils::tryStringifyMessage(const messageUtils::Message& message, const std::string& messageFormat) {
    if (messageFormat != Message::defaultStringifiedMessageFormat) {
        return std::nullopt;
    }
    return std::make_optional(fmt::format(Message::defaultStringifiedMessageFormat, message.position.line, message.position.column, message.text));
}

std::optional<std::string> messageUtils::tryStringifyMessages(const std::vector<Message>& messages, char const* messageDelimiter) {
    std::vector<std::string> stringifiedMessages(messages.size(), "");

    bool stringificationOk = true;
    for (std::size_t i = 0; i < messages.size() && stringificationOk; ++i) {
        if (const auto& stringifiedMsg = tryStringifyMessage(messages.at(i)); stringifiedMsg.has_value()) {
            stringifiedMessages.at(i) = *stringifiedMsg;
        } else {
            stringificationOk = false;
        }
    }

    if (!stringificationOk) {
        return std::nullopt;
    }

    std::ostringstream errorsConcatinatedBuffer;
    std::copy(stringifiedMessages.cbegin(), stringifiedMessages.cend(), infix_ostream_iterator<std::string>(errorsConcatinatedBuffer, messageDelimiter));
    return errorsConcatinatedBuffer.str();
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
