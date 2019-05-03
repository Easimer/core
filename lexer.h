#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace core {
    enum class tok_t {
        invalid = 0,
        unknown = 1,
        
        fn = 2,
        ext = 3,
        
        identifier = 4,
        literal = 5,
        paren_open = 6,
        paren_close = 7,
        eol = 8, // end of line
        eob = 9, // end of block
        
        max
    };
    
    core::tok_t get_token(std::string& buf, input_file& f);
    
    struct token {
        tok_t type;
        std::string value;
    };
    
    struct line {
        std::vector<token> tokens;
    };
    
    struct block {
        std::vector<line> lines;
    };
}