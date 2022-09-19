#include "core/syrec/program.hpp"
#include "parser/Parser.h"

namespace syrec {

    bool program::readFile(const std::string& filename, const ReadProgramSettings settings, std::string* error) {
        *error = read(filename, settings);
        return error->empty();
    }

    unsigned char* read_and_buffer_file_content(const std::string& filename, std::streampos* file_content_size) {
        *file_content_size = 0;
        std::basic_ifstream<unsigned char> is(filename.c_str(), std::ifstream::in | std::ifstream::binary);
        if (is.is_open()) {
            is.seekg(0, is.end);
            *file_content_size = is.tellg();
            is.seekg(0, is.beg);

            const auto input_circuit_content_buffer = new unsigned char[*file_content_size];
            is.read(input_circuit_content_buffer, *file_content_size);
            return input_circuit_content_buffer;
        } else {
            *file_content_size = -1;
            return nullptr;
        }
    }

    std::string program::read(const std::string& filename, const ReadProgramSettings settings) {
        std::streampos input_circuit_file_length    = 0;
        const auto     input_circuit_content_buffer = read_and_buffer_file_content(filename, &input_circuit_file_length);
        if (input_circuit_content_buffer != nullptr) {
            auto scanner = Scanner(input_circuit_content_buffer, input_circuit_file_length);
            auto parser  = Parser(&scanner);
            parser.Parse();

            if (parser.errors->count) {
                return "Error while parsing input circuit from file @" + filename;
            } else {
                this->modulesVec = parser.modules;
                return {};
            }
        } else {
            return "Cannot open given circuit file @ " + filename;
        }
    }

} // namespace syrec