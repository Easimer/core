#pragma once

#include "lexer.h"
#include "ast.h"

// That which converts the tokens into the AST

namespace core {
    up<ast_expression> parse(token_stream& t);
}