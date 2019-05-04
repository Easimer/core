#include <iostream>
#include <list>

#include "lexer.h"
#include "parser.h"

int main(int argc, char** argv) {
    if(argc > 1) {
        input_file f(argv[1]);
        core::token t;
        core::token_stream ts;
        
        while(!is_eof(f)) {
            t = core::get_token(f);
            if(t.second.size()) {
                printf("%s %d\n", t.second.c_str(), (int)t.first);
                ts.c.push_back(std::move(t));
            }
        }
        printf("\n");
        
        while(!ts.empty()) {
            auto expr = core::parse(ts);
            if(expr) {
                expr->dump();
            }
        }
    }
    return 0;
}