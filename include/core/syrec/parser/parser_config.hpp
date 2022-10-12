#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

namespace parser {
    struct ParserConfig {
        unsigned int cDefaultSignalWidth;

        explicit ParserConfig(const unsigned int defaultSignalWidth = 32U):
            cDefaultSignalWidth(defaultSignalWidth){};

        
        ParserConfig& operator=(const ParserConfig& copy) {
            cDefaultSignalWidth = copy.cDefaultSignalWidth;
            return *this;
        }
    };
}

#endif