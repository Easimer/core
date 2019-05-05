#include "log.h"
#include "ast.h"
#include <cstdarg>

void log_err(const core::token_stream& ts, const char* pszFormat, ...) {
    va_list va;
    
    va_start(va, pszFormat);
    
    fprintf(stderr, "\033[91mError\033[0m [%d:%d]: ", ts.line() + 1, ts.col() + 1);
    vfprintf(stderr, pszFormat, va);
    fprintf(stderr, "\n");
    
    va_end(va);
}

void log_err(const core::ast_expression* expr, const char* pszFormat, ...) {
    va_list va;
    
    va_start(va, pszFormat);
    
    fprintf(stderr, "\033[91mError\033[0m [%d:%d]: ", expr->line + 1, expr->col + 1);
    vfprintf(stderr, pszFormat, va);
    fprintf(stderr, "\n");
    
    va_end(va);
}

void log_warn(const core::ast_expression* expr, const char* pszFormat, ...) {
    va_list va;
    
    va_start(va, pszFormat);
    
    fprintf(stderr, "\033[93mWarning\033[0m [%d:%d]: ", expr->line + 1, expr->col + 1);
    vfprintf(stderr, pszFormat, va);
    fprintf(stderr, "\n");
    
    va_end(va);
}