#ifndef TEXT_UTILS_HPP
#define TEXT_UTILS_HPP

#include <cstdlib>
#include <string>

namespace syrec {
    [[nodiscard]] static std::string convert_to_uniform_text_format(const wchar_t* text) {
        const std::wstring wide_text_as_string(text);
        const std::size_t        size_of_wide_text = wide_text_as_string.size();

        if (size_of_wide_text == 0 || size_of_wide_text == SIZE_MAX) {
            return "";
        }

        char*             test_1 = new char[size_of_wide_text + 1];
        std::size_t       num_characters_converted;
        
        if (!wcstombs_s(&num_characters_converted, test_1, size_of_wide_text + 1, text, size_of_wide_text)) {
            return test_1;
        } else {
            return "";
        }
    }
}

#endif