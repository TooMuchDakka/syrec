#ifndef MESSAGE_UTILS_HPP
#define MESSAGE_UTILS_HPP
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <fmt/core.h>

namespace messageUtils {
    struct Message {
        inline static const std::string defaultStringifiedMessageFormat = "-- line {0:d} col {1:d}: {2:s}";

        enum ErrorCategory {
            Syntax,
            Semantic
        };

        struct Position {
            std::size_t line;
            std::size_t column;

            explicit Position(const std::size_t line, const std::size_t column):
                line(line), column(column) {
            }

            /**
             * \brief Determines the order of the given instance and the provided position
             * \param otherPosition The position with which the current instance is compare to
             * \return 1 if the given instance is larger, 0 is they are equal and -1 if the current instance is smaller than the other position.
             */
            [[nodiscard]] int compare(const Position& otherPosition) const;
        };

        enum Severity {
            Error,
            Warning,
            Information
        };

        ErrorCategory category;
        Position    position;
        Severity    severity;
        std::string text;
    };

    // TODO: Log to file ?
    class MessageFactory {
    public:
        MessageFactory(std::size_t fallbackErrorLinePosition, std::size_t fallbackErrorColumnPosition):
            fallBackErrorLinePosition(fallbackErrorLinePosition), fallBackErrorColumnPosition(fallbackErrorColumnPosition) {}

        MessageFactory(): MessageFactory(0, 0) {}

        [[nodiscard]] static Message createMessage(Message::ErrorCategory errorCategory, Message::Position position, Message::Severity severity, const std::string_view& message);
        [[nodiscard]] static Message createError(Message::ErrorCategory errorCategory, Message::Position position, const std::string_view& message);

        template<typename ...T>
        [[nodiscard]] Message createMessage(Message::ErrorCategory errorCategory, Message::Position position, Message::Severity severity, const std::string_view& format, T&&... args) const {
            const auto& generatedErrorMsg = fmt::format(format, std::forward<T>(args)...);
            return createMessage(errorCategory, position, severity, generatedErrorMsg);
        }

        template<typename ...T>
        [[nodiscard]] Message createError(Message::ErrorCategory errorCategory, Message::Position position, const std::string_view& messageFormat, T&&... args) const {
            return createMessage(errorCategory, position, Message::Severity::Error, messageFormat, std::forward<T>(args));
        }
        
    private:
        std::size_t fallBackErrorLinePosition;
        std::size_t fallBackErrorColumnPosition;
    };

    [[nodiscard]] std::optional<std::string> tryStringifyMessage(const Message& message, const std::string_view& messageFormat = Message::defaultStringifiedMessageFormat);
    [[nodiscard]] std::optional<std::string> tryStringifyMessages(const std::vector<Message>& messages, char const* messageDelimiter = "\n");
    [[nodiscard]] std::vector<std::string>   tryDeserializeStringifiedMessagesFromString(const std::string_view& stringifiedMessages, char const* expectedMessageDelimiter = "\n");

}
#endif