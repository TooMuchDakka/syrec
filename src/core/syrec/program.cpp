#include "core/syrec/program.hpp"
#include "parser/Parser.h"

namespace syrec {

    bool program::read_file(const std::string& filename, const read_program_settings settings, std::string* error) {
        std::string content, line;

        std::ifstream is;
        is.open(filename.c_str(), std::ios::in);

        while (getline(is, line)) {
            content += line + '\n';
        }

        return read_program_from_string(content, settings, error);
    }

    unsigned char *read_and_buffer_file_content(const std::string& filename, std::streampos *file_content_size) {
        *file_content_size = 0;
        std::basic_ifstream<unsigned char> is(filename.c_str(), std::ifstream::in | std::ifstream::binary);
        if (is.is_open()) {
            is.seekg(0, is.end);
            *file_content_size = is.tellg();
            is.seekg(0, is.beg);

            const auto input_circuit_content_buffer = new unsigned char[*file_content_size];
            is.read(input_circuit_content_buffer, *file_content_size);
            return input_circuit_content_buffer;
        }
        else {
            *file_content_size = -1;
            return nullptr;
        }
    }

    std::string program::read(const std::string& filename, const read_program_settings settings) {
        std::streampos input_circuit_file_length    = 0;
        const auto     input_circuit_content_buffer = read_and_buffer_file_content(filename, &input_circuit_file_length);
        if (input_circuit_content_buffer != nullptr) {
            auto scanner = Scanner(input_circuit_content_buffer, input_circuit_file_length);
            auto parser  = Parser(&scanner);
            parser.Parse();

            if (parser.errors->count) {
                return "Error while parsing input circuit from file @" + filename;
            } else {
                return {};
            }
        } else {
            return "Cannot open given circuit file @ " + filename;
        }

        /*
        std::basic_ifstream<unsigned char> is(filename.c_str(), std::ifstream::in | std::ifstream::binary);
        if (is.is_open()) {
            is.seekg(0, is.end);
            const std::streampos input_circuit_file_length = is.tellg();
            is.seekg(0, is.beg);

            
            const auto input_circuit_content_buffer = new unsigned char[input_circuit_file_length];
            is.read(input_circuit_content_buffer, input_circuit_file_length);
            auto scanner = Scanner(input_circuit_content_buffer, input_circuit_file_length);
            auto parser  = Parser(&scanner);
            parser.Parse();

            if (parser.errors->count) {
                return "Error while parsing input circuit from file @" + filename;
            } else {
                return {};
            }
        } else {
            return "Cannot open given circuit file @ " + filename;
        }
        */


        /*
        std::string error_message;
        if (!(read_file(filename, settings, &error_message))) {
            return error_message;
        } else {
            return {};
        }
        */
    }

} // namespace syrec
