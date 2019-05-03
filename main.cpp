#include "parser.h"
#include "lexer.h"
#include <iostream>

int main(int argc, char** argv) {
    input_file f(argv[1]);
    std::string buf;
    core::tok_t t;
    while(!is_eof(f)) {
        t = core::get_token(buf, f);
        fprintf(stderr, "type %d buf '%s'\n", (int)t, buf.c_str());
    }
    fprintf(stderr, "eof\n");
    
    return 0;
}