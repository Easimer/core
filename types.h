#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <memory>

#define SHOW_BLOCK_MSG 1

namespace core {
    using u8  = uint8_t;
    using s8  = int8_t;
    using u16 = uint16_t;
    using s16 = int16_t;
    using u32 = uint32_t;
    using s32 = int32_t;
    template<typename T>
        using up = std::unique_ptr<T>;
}

struct input_file {
    FILE* fd;
    std::string path;
    char last_char;
    
    input_file(const std::string& path) : path(path), last_char(0) {
        fd = fopen(path.c_str(), "r");
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