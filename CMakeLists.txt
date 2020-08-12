cmake_minimum_required(VERSION 3.14)

project(elf_randomizer)

add_executable(elf_randomizer elf_randomizer.cpp main.cpp)

target_include_directories(elf_randomizer PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                            )

