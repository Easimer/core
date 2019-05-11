#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <memory>

#define SHOW_BLOCK_MSG 0

namespace core {
    // LLVM State
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
        
        bool current_function_pure = false;
        
        std::unordered_map<llvm::Function*, bool> func_is_pure;
        
        llvm_ctx(const char* pszSource, const char* pszModuleName) :
        builder(ctx), module(pszModuleName, ctx), dbuilder(module), compile_unit(dbuilder.createCompileUnit(llvm::dwarf::DW_LANG_C, dbuilder.createFile(pszSource, "."), "corec", 0, "", 0)), di_scope(nullptr) {
            // Setup debug types
            di_types["real"] = dbuilder.createBasicType("real", 64, llvm::dwarf::DW_ATE_float);
            di_types["int"] = dbuilder.createBasicType("int", 64, llvm::dwarf::DW_ATE_signed);
            di_types["bool"] = dbuilder.createBasicType("bool", 1, llvm::dwarf::DW_ATE_unsigned);
            di_types["_unknown"] = dbuilder.createUnspecifiedType("_unknown");
        }
    };
    
    using u8  = uint8_t;
    using s8  = int8_t;
    using u16 = uint16_t;
    using s16 = int16_t;
    using u32 = uint32_t;
    using s32 = int32_t;
    template<typename T>
        using up = std::unique_ptr<T>;
    template<typename T>
        using sp = std::shared_ptr<T>;
}

struct input_file {
    FILE *fd;
    std::string path;
    char last_char;
    
    int line, col;
    
    input_file(const std::string& path) : path(path), last_char(0), line(0), col(0) {
        fd = fopen(path.c_str(), "r");
    }
    
    ~input_file() {
        if(fd) {
            fclose(fd);
        }
    }
};

#define is_eof(f) (feof(f.fd))

template<typename Container>
struct front_t {
    constexpr front_t(Container& tar) : container(tar) {}
    auto& operator*() {
        return container.front();
    }
    auto& operator->() {
        return container.front();
    }
    auto& operator++() {
        container.pop_front();
        return container.front();
    }
    auto operator++(int) {
        auto ret = container.front();
        container.pop_front();
        return ret;
    }
    
    Container& container;
};

static int level = 0;

struct block_msg {
    block_msg(const char* pszBlockName) : m_pszBlockName(pszBlockName) {
#if SHOW_BLOCK_MSG
        for(int i = 0; i < level; i++) printf("\t");
        printf("Entered %s\n", m_pszBlockName);
        level++;
#endif
    }
    
    ~block_msg() {
#if SHOW_BLOCK_MSG
        level--;
        for(int i = 0; i < level; i++) printf("\t");
        printf("Exited %s\n", m_pszBlockName);
#endif
    }
    
    const char* m_pszBlockName;
};