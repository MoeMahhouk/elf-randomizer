#include "elf_randomizer.hpp"
using namespace elf_randomizer;


void Elf_randomizer::create_shuffled_elf(std::string &new_elf_name)
{
    void *new_elf_file = calloc(1, _elf_size);
    memcpy(new_elf_file, _elf_file, _elf_size);

    if(new_elf_file == NULL)
    {
        printf("copying didnt work!!!\n");
        abort();
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr*) new_elf_file;
    Elf64_Shdr *shdr = (Elf64_Shdr*)(new_elf_file + ehdr->e_shoff);
    int shnum = ehdr->e_shnum;

    Elf64_Shdr *sh_strtab = &shdr[ehdr->e_shstrndx];
    const char *const sh_strtab_p = (char*)new_elf_file + sh_strtab->sh_offset;

    std::vector<Elf64_Shdr> shuffled_shdrs = shuffle_section_headers();

    for (size_t i = 0; i < shnum; i++)
    {
        //find and fix the string table index in the elf header
        std::string current_shdr_name = std::string(sh_strtab_p + shuffled_shdrs[i].sh_name);
        auto itr = _shdr_index_map.find(current_shdr_name);
        if(itr != _shdr_index_map.end())
        {
            itr->second.own_index_new = i;
        }
        if(current_shdr_name.compare(str_table_name) == 0) 
        {
            ehdr->e_shstrndx = i;
        }
    }

    update_indexs_map();
    Elf64_Shdr *symtab_ptr = NULL;
    size_t ttl_needed_size = ehdr->e_ehsize;

    for (size_t i = 0; i < shnum; i++)
    {
        memcpy(&shdr[i], &shuffled_shdrs[i], sizeof(Elf64_Shdr));
        std::string current_sec_name = std::string(sh_strtab_p + shdr[i].sh_name);
        std::map<uint32_t, uint32_t>::iterator it = indexs_map.find(shdr[i].sh_link); 

        //fetch the symtab section to fix the indexes later
        if(shdr[i].sh_type == SHT_SYMTAB || shdr[i].sh_type == SHT_DYNSYM)
        {
            symtab_ptr = &shdr[i];
        }

        // update the link indexes for each shuffled section
        if (it != indexs_map.end())
        {
   
            shdr[i].sh_link = it->second;
         
        } else 
        {
            printf("map entry not found");
            exit(1);
        }

        // update the info index in case it should reference a relocatable section
        if(shdr[i].sh_info != 0 && (shdr[i].sh_type == SHT_REL || shdr[i].sh_type == SHT_RELA))
        {

            it = indexs_map.find(shdr[i].sh_info);
            if (it != indexs_map.end())
            {
                shdr[i].sh_info = it->second;
            } else
            {
                printf("map info entry not found");
                exit(1);
            } 
        }
        if(i != 0) 
        {
            auto needed_alignment_bytes = (shdr[i].sh_addralign - (ttl_needed_size % shdr[i].sh_addralign)) % shdr[i].sh_addralign;
            ttl_needed_size += needed_alignment_bytes + shdr[i].sh_size;
        }
    }
    // the total needed size is needed to create new binary where the section are shuffled 
    auto needed_alignment_bytes = (SH_TAB_ALIGN - (ttl_needed_size % SH_TAB_ALIGN)) % SH_TAB_ALIGN;
    ttl_needed_size += needed_alignment_bytes;

    // fix symbol table indexes to the new ones
    auto total_syms = symtab_ptr->sh_size / sizeof(Elf64_Sym);
    auto syms_data = (Elf64_Sym*)(new_elf_file + symtab_ptr->sh_offset);

    for (int i = 0; i < total_syms; ++i) 
    {
        auto current_st_shndx = syms_data[i].st_shndx;
        if(current_st_shndx > 0 && current_st_shndx < old_shdrs.size())
        {
            std::map<uint32_t, uint32_t>::iterator it = indexs_map.find(current_st_shndx); 
            if (it != indexs_map.end())
            {
                syms_data[i].st_shndx = it->second;
            }
        }        
    }

    void *shuffled_elf_file = NULL;
    int diff = ttl_needed_size - ehdr->e_shoff;
    size_t new_shuffled_elf_size = _elf_size + diff;
    shuffled_elf_file = calloc(1, new_shuffled_elf_size);

    size_t curr_offset = 0;
    for (size_t i = 0; i < shnum; i++)
    {
        if(i == 0)
        {
            memcpy(shuffled_elf_file, new_elf_file, ehdr->e_ehsize);
            curr_offset += ehdr->e_ehsize;
            Elf64_Ehdr *shuffled_ehdr = (Elf64_Ehdr*) shuffled_elf_file;
            shuffled_ehdr->e_shoff = ttl_needed_size;

        } else
        {
            
            auto needed_align_bytes = (shdr[i].sh_addralign - (curr_offset % shdr[i].sh_addralign)) % shdr[i].sh_addralign;
            curr_offset += needed_align_bytes;
            memcpy(shuffled_elf_file + curr_offset, new_elf_file + shdr[i].sh_offset, shdr[i].sh_size);
            shdr[i].sh_offset = curr_offset; //change the offsets to the new ones
            curr_offset += shdr[i].sh_size;
        }        
    }
    
    //copy the rest as they are (already shuffled with updated indexes in the section header table and string table)
    memcpy(shuffled_elf_file + ttl_needed_size, new_elf_file + ehdr->e_shoff, _elf_size - ehdr->e_shoff);

    std::ofstream outfile (new_elf_name, std::ofstream::binary);
    outfile.write((const char *)shuffled_elf_file, new_shuffled_elf_size);
    outfile.close();
}        


void Elf_randomizer::init() 
{
    int fd, i;
    struct stat st;

    if ((fd = open(_elf_path.c_str(), O_RDWR)) < 0) {
        printf("Err: open\n");
        exit(-1);
    }
    if (fstat(fd, &st) < 0) {
        printf("Err: fstat\n");
        exit(-1);
    }
    _elf_size = st.st_size; 
    _elf_file = static_cast<uint8_t*>(mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (_elf_file == MAP_FAILED) {
        printf("Err: mmap\n");
        exit(-1);
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)_elf_file;
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        printf("Only 64-bit files supported\n");
        exit(1);
    }

    if(ehdr->e_type != ET_REL)
    {
        printf("Only supported for ELF of type relocatable file!\n");
        exit(1);
    }

    Elf64_Shdr *shdr = (Elf64_Shdr*)(_elf_file + ehdr->e_shoff);
    int shnum = ehdr->e_shnum;

    Elf64_Shdr *sh_strtab = &shdr[ehdr->e_shstrndx];
    const char *const sh_strtab_p = (char*)_elf_file + sh_strtab->sh_offset;


    for (uint32_t i = 0; i < shnum; ++i) 
    {
        std::string current_sec_name = std::string(sh_strtab_p + shdr[i].sh_name);
        old_shdrs.push_back(shdr[i]);
        _shdr_index_map.insert(std::pair<std::string, index_map_entry_t>(current_sec_name, {i,i}));
        indexs_map.insert(std::pair<uint32_t, uint32_t>(i,i));     
    }

    // get the used string table name
    str_table_name = std::string(sh_strtab_p + shdr[ehdr->e_shstrndx].sh_name);
}



//ToDo maybe its easier if give the array of the new section headers and do a simple comparision between the entries to get the new indexes
void Elf_randomizer::update_indexs_map()
{
    //it1 is for the section header name mapped to the new index 
    //index_itr is for mapping the old section header index to the new one 
    std::map<std::string, index_map_entry_t>::iterator it1; 
    std::map<uint32_t, uint32_t>::iterator index_itr;


    for (it1 = _shdr_index_map.begin(); it1 != _shdr_index_map.end(); it1++ )
    {
        if((index_itr = indexs_map.find(it1->second.own_index_old)) != indexs_map.end())
        {
            index_itr->second = it1->second.own_index_new; // set the new index correctly
        } else 
        {
            printf("ERROR: important section header map entry not found !\n");
            exit(1);
        }
    
    }  
}

std::vector<Elf64_Shdr> Elf_randomizer::shuffle_section_headers()
{
    std::vector<Elf64_Shdr> shuffled_shdrs(old_shdrs);
    std::random_device rd; 
    auto rng = std::default_random_engine {rd()};
    std::shuffle(std::begin(shuffled_shdrs)+1, std::end(shuffled_shdrs), rng);
    return shuffled_shdrs;
}

