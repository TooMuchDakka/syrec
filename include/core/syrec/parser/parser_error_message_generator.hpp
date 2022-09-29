#ifndef PARSER_ERROR_MESSAGE_GENERATOR_HPP
#define PARSER_ERROR_MESSAGE_GENERATOR_HPP

#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

enum error_message {
    DEFAULT
};

// https://stackoverflow.com/questions/1657883/variable-number-of-arguments-in-c
// https://refactoring.guru/design-patterns/builder/cpp/example
namespace parser {
    class parser_error_message_generator {
        struct format_specifier_position {
            std::size_t start_position;
            std::size_t length;

            explicit format_specifier_position(const std::size_t start_position, const std::size_t length) {
                this->start_position = start_position;
                this->length         = length;
            }
        };

        struct error_message_generation_config {
            std::string                          error_msg_format;
            std::vector<format_specifier_position> start_and_end_indices_of_format_specifiers_in_msg_format;

            explicit error_message_generation_config(const std::string &error_msg_format) {
                this->error_msg_format                                     = error_msg_format;
                this->start_and_end_indices_of_format_specifiers_in_msg_format = find_format_specifiers(this->error_msg_format);
            }

        private:
            [[nodiscard]] static std::vector<format_specifier_position> find_format_specifiers(const std::string &error_msg_format) {
                std::size_t search_start_position = 0;
                std::vector<format_specifier_position> found_format_specifiers;

                while (search_start_position != std::string::npos) {
                    const std::size_t next_format_specifier_start_idx = error_msg_format.find_first_of('(', search_start_position);
                    std::size_t next_format_specifier_end_idx   = std::string::npos;

                    if (next_format_specifier_start_idx != std::string::npos) {
                        next_format_specifier_end_idx = error_msg_format.find_first_of(')', next_format_specifier_start_idx);
                    }

                    const std::size_t format_specifier_length = next_format_specifier_end_idx - next_format_specifier_start_idx + 1;
                    if (next_format_specifier_start_idx != std::string::npos && next_format_specifier_end_idx != std::string::npos
                        && format_specifier_length > 2) {
                        found_format_specifiers.emplace_back(format_specifier_position(next_format_specifier_start_idx, format_specifier_length));
                    }
                    search_start_position = next_format_specifier_end_idx;
                }
                return found_format_specifiers;
            }
        };

    public:
        parser_error_message_generator() {
            error_message_config_lookup = init_error_message_configs();
        }

        template<typename... Args>
        std::string build_message(const error_message error_message, Args... args) {
            const auto found_error_message_config = error_message_config_lookup.find(error_message);
            if (found_error_message_config == error_message_config_lookup.end()) {
                return "";
            } 

            const error_message_generation_config config                       = found_error_message_config->second;
            const unsigned int number_of_user_provided_args = sizeof...(args);
            if (number_of_user_provided_args != config.start_and_end_indices_of_format_specifiers_in_msg_format.size()) {
                return "";
            }

            return build_message_for_message_config(config);
        }

    private:
        std::map<error_message, error_message_generation_config> error_message_config_lookup;
        
        template<typename T>
        static void replace_format_specifier_with_value(std::ostringstream& oss, const std::string_view format_string_part, const T& value) {
            oss << format_string_part << value;
        }

        [[nodiscard]] static std::vector<std::string_view> split_string_at_format_specifiers(std::string_view format_string, const std::vector<format_specifier_position> &format_specifier_positions);

        template<typename... Args>
        [[nodiscard]] static std::string                          build_message_for_message_config(const error_message_generation_config& config, Args... args) {
            auto error_msg_parts_split_at_specifiers = split_string_at_format_specifiers(config.error_msg_format, config.start_and_end_indices_of_format_specifiers_in_msg_format);

            std::ostringstream oss{};
            int                format_specifier_index = 0;
            (replace_format_specifier_with_value(oss, error_msg_parts_split_at_specifiers[format_specifier_index++], args), ...);
            return oss.str();
        }

        [[nodiscard]] static std::map<error_message, error_message_generation_config> init_error_message_configs();
    };
}

#endif