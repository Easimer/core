" Vim syntax file
" " Language: Core
" " Maintainer: Daniel Meszaros
" " Latest Revision: May 5 2019

if exists("b:current_syntax")
  finish
endif

syn keyword corFunction fn
syn keyword corExtern extern
syn keyword corReturn return
syn keyword corConditional if then
syn keyword corType bool real
syn match corFunctionName '^(?:fn)\s+(\S+)(?:[(])'

hi def link corFunction Keyword
hi def link corExtern Keyword
hi def link corReturn Keyword
hi def link corConditional Conditional
hi def link corType Type

