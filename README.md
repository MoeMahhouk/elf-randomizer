# Purpose

This creates a shuffled version of an arbitrary relocatable file (ELF). \
It will be then used in the sgx-dl framework as an extra randomisation layer before dynamically loading the contents 

# Prerequisites

- CMake Version > 3.14   (ps: it can be lowered down since it doesnt require any special cmake new features)


# Setup

- mkdir build
- cd build
- cmake ..
- make

# Usage

- ./elf-randomizer [relocatable_file_path] [optional: output_file_name]