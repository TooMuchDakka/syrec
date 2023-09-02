#ifndef MESSAGE_UTILS_HPP
#define MESSAGE_UTILS_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <fmt/core.h>

namespace messageUtils {
    struct Message {
        inline static const std::string defaultStringifiedMessageFormat = "-- line {0:d} col {1:d}: {2:s}";

        struct Position {
            std::size_t line;
            std::size_t column;

            explicit Position(const std::size_t line, const std::size_t column):
                line(line), column(column) {
            }
        };

        enum Severity {
            Error,
            Warning,
            Information
        };

        Position    position;
        Severity    severity;
        std::string text;
    };

    // TODO: Offer types alternative for message creation: 
    class MessageFactory {
    public:
        MessageFactory(std::size_t fallbackErrorLinePosition, std::size_t fallbackErrorColumnPosition):
            fallBackErrorLinePosition(fallbackErrorLinePosition), fallBackErrorColumnPosition(fallbackErrorColumnPosition) {}

        MessageFactory(): MessageFactory(0, 0) {}

        [[nodiscard]] static Message createMessage(Message::Position position, Message::Severity severity, const std::string& message);

        template<typename ...T>
        [[nodiscard]] Message createMessage(Message::Position position, Message::Severity severity, const std::string& format, T&&... args) const {
            const auto& generatedErrorMsg = fmt::format(format, std::forward<T>(args)...);
            return createMessage(position, severity, generatedErrorMsg);
        }
        
    private:
        std::size_t fallBackErrorLinePosition;
        std::size_t fallBackErrorColumnPosition;
    };

    [[nodiscard]] std::optional<std::string> tryStringifyMessage(const Message& message, const std::string& messageFormat = Message::defaultStringifiedMessageFormat);
    [[nodiscard]] std::optional<std::string> tryStringifyMessages(const std::vector<Message>& messages, char const* messageDelimiter = "\n");
    [[nodiscard]] std::vector<std::string>   tryDeserializeStringifiedMessagesFromString(const std::string_view& stringifiedMessages, char const* expectedMessageDelimiter = "\n");

}
#endif