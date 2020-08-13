// MIT License

// Copyright (c) 2018 finixbit

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef H_ELF_PARSER
#define H_ELF_PARSER

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <map>
#include <cstdio>
#include <fcntl.h>    /* O_RDONLY */
#include <sys/stat.h> /* For the size of the file. , fstat */
#include <sys/mman.h> /* mmap, MAP_PRIVATE */
#include <vector>
#include <elf.h>      // Elf64_Shdr
#include <fcntl.h>
#include <algorithm>
#include <random>
#include <fstream>


#define SH_TAB_ALIGN 8
namespace elf_randomizer {

typedef struct 
{
    uint32_t own_index_old;
    uint32_t own_index_new;
} index_map_entry_t;


class Elf_randomizer {
    public:
        Elf_randomizer (std::string &elf_path): _elf_path{elf_path}
        {   
            init();
        }
        void create_shuffled_elf(std::string &new_elf_name);        
        
    private:
        void init();
        std::vector<Elf64_Shdr> shuffle_section_headers();
        void update_indexs_map();

        std::vector<Elf64_Shdr> old_shdrs;
        std::string _elf_path; 
        uint8_t *_elf_file;
        size_t _elf_size;
        std::string str_table_name;
        std::map<std::string, index_map_entry_t> _shdr_index_map;
        std::map<uint32_t, uint32_t> indexs_map;
};

}
#endif
