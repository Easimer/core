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
#include <llvm/IR/DIBuilder.h>

#include "types.h"

#define OVERRIDE_GEN_IR() virtual llvm::Value* generate_ir(llvm_ctx& ctx) override

namespace core {
    // LLVM state
    struct llvm_ctx {
        llvm::LLVMContext ctx;
        llvm::IRBuilder<> builder;
        llvm::Module module;
        
        llvm::DIBuilder dbuilder;
        llvm::DICompileUnit* compile_unit;
        
        std::unordered_map<std::string, llvm::AllocaInst*> globals;
        std::unordered_map<std::string, llvm::AllocaInst*> locals;
        
        std::unordered_map<llvm::Function*, llvm::DISubroutineType*> di_func_sigs;
        std::unordered_map<std::string, llvm::DIType*> di_types;
        llvm::DIScope* di_scope;
        
        llvm_ctx(const char* pszSource, const char* pszModuleName) :
        builder(ctx), module(pszModuleName, ctx), dbuilder(module), compile_unit(dbuilder.createCompileUnit(llvm::dwarf::DW_LANG_C, dbuilder.createFile(pszSource, "."), "corec", 0, "", 0)), di_scope(nullptr) {
            // Setup debug types
            di_types["real"] = dbuilder.createBasicType("real", 64, llvm::dwarf::DW_ATE_float);
            di_types["int"] = dbuilder.createBasicType("int", 64, llvm::dwarf::DW_ATE_signed);
            di_types["bool"] = dbuilder.createBasicType("bool", 1, llvm::dwarf::DW_ATE_unsigned);
            di_types["_unknown"] = dbuilder.createUnspecifiedType("_unknown");
        }
    };
    
    class ast_expression {
        public:
        int line = 0, col = 0;
        
        virtual ~ast_expression() {}
        
        virtual void dump() = 0;
        
        virtual llvm::Value* generate_ir(llvm_ctx& ctx) { return nullptr; };
    };
    
    class ast_literal : public ast_expression {
        public:
        ast_literal(const std::string& value) : value(value) {}
        
        std::string value;
        bool is_real = false;
        bool is_int = false;
        bool is_bool = false;
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