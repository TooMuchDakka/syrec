#include "parser_error_message_generator.hpp"

namespace syrec {

    [[nodiscard]] std::vector<std::string_view> parser_error_message_generator::split_string_at_format_specifiers(const std::string_view format_string, const std::vector<format_specifier_position>& format_specifier_positions) {
        std::vector<std::string_view> message_string_parts;
        std::size_t split_start_position                       = 0;

        for (const format_specifier_position format_specifier_position: format_specifier_positions) {
            const std::size_t index_of_end_of_specifier = format_specifier_position.start_position + format_specifier_position.length + 1;
            const std::size_t length_of_message_part = format_specifier_position.start_position - split_start_position;

            message_string_parts.push_back(format_string.substr(split_start_position, length_of_message_part));
            split_start_position = index_of_end_of_specifier;
        }
        return message_string_parts;
    }

    [[nodiscard]] std::map<error_message, parser_error_message_generator::error_message_generation_config> parser_error_message_generator::init_error_message_configs() {
        return { {DEFAULT, error_message_generation_config("This is a test {0} string to shwo if its {1} working, {2}, test {3}")}};
    }
}