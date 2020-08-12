#include "elf_randomizer.hpp"
#include <cstring>


int main(int argc, char* argv[]) {
    char usage_banner[] = "usage: ./sections [<input_elf_name>] [optional: output_elf_name]\n";
    if(argc < 2) {
        std::cerr << usage_banner;
        return -1;
    }

    std::string elf_file((std::string)argv[1]);
    
    std::string new_elf_name(elf_file);
    new_elf_name.insert(0, "new_");

    if(argc > 2)
    {
        new_elf_name = std::string(argv[2]);
    } 

    elf_randomizer::Elf_randomizer elf_randomizer(elf_file);
    elf_randomizer.create_shuffled_elf(new_elf_name);

    return 0;
}