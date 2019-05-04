#pragma once

#include <vector>
#include <unordered_map>

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

#include "types.h"
#include "codegen.h"

#define OVERRIDE_GEN_IR() virtual llvm::Value* generate_ir(llvm_ctx& ctx) override

namespace core {
    // LLVM state
    struct llvm_ctx {
        llvm::LLVMContext ctx;
        llvm::IRBuilder<> builder;
        llvm::Module module;
        
        std::unordered_map<std::string, llvm::AllocaInst*> globals;
        std::unordered_map<std::string, llvm::AllocaInst*> locals;
        
        llvm_ctx(const char* pszModuleName) :
        builder(ctx), module(pszModuleName, ctx) {}
    };
    
    class ast_expression {
        public:
        int line = 0, col = 0;
        
        virtual ~ast_expression() {}
        
        virtual void dump() = 0;
        
        virtual llvm::Value* generate_ir(llvm_ctx& ctx) { return nullptr; };
    };
    
    class ast_literal : public ast_expression {};
    
    class ast_real : public ast_literal {
        public:
        float value;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_boolean : public ast_literal {
        public:
        bool value;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_identifier : public ast_expression {
        public:
        
        ast_identifier(const char* pszName);
        
        char name[128];
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_declaration : public ast_expression {
        public:
        
        up<ast_identifier> identifier;
        std::string type;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_prototype : public ast_expression {
        public:
        up<ast_expression> name;
        std::vector<ast_declaration> args;
        up<ast_identifier> type;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_function : public ast_expression {
        public:
        up<ast_prototype> prototype;
        std::vector<up<ast_expression>> lines;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_binary_op : public ast_expression {
        public:
        
        ast_binary_op(char op, up<ast_expression> lhs, up<ast_expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        
        char op;
        up<ast_expression> lhs, rhs;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_function_call : public ast_expression {
        public:
        up<ast_identifier> name;
        std::vector<up<ast_expression>> args;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_branching : public ast_expression {
        public:
        up<ast_expression> condition;
        up<ast_expression> line;
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
}