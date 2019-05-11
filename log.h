#pragma once

#include <cstdio>
#include "lexer.h"
#include "ast.h"
void log_err(const core::token_stream& ts, const char* pszFormat, ...);
void log_err(const core::ast_expression* expr, const char* pszFormat, ...);
void log_warn(const core::ast_expression* expr, const char* pszFormat, ...);
void log_warn(const core::token_stream& ts, const char* pszFormat, ...);