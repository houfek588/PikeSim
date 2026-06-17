cmake_minimum_required(VERSION 3.20)

# ŘEŠENÍ PRO NOVÉ VERZE CMAKE (CLion 2025+):
# Nutí CMake považovat všechny závislosti (včetně Raylibu) za kompatibilní minimálně s verzí 3.5.
# Tím se vyhneme fatální chybě "Compatibility with CMake < 3.5 has been removed".
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Aktivujeme politiku CMP0135 na úroveň NEW, což definitivně vyřeší varování ohledně DOWNLOAD_EXTRACT_TIMESTAMP
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(PikeSquareSimulator CXX)

# Nastavení standardu C++17 (vyžadováno pro struktury a matematické knihovny)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- AUTOMATICKÉ STAŽENÍ A NALINKOVÁNÍ RAYLIB ---
include(FetchContent)

# Deklarujeme Raylib verze 5.0 (stabilní)
FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)

# Vypneme sestavení příkladů a her uvnitř Raylibu, abychom urychlili kompilaci
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SUPPORT_GIF_RECORDING OFF CACHE BOOL "" FORCE)

# Stažení a konfigurace
FetchContent_MakeAvailable(raylib)

# --- DEFINICE EXECUTABLE ---
add_executable(PikeSquareSimulator main.cpp)

# Propojení naší aplikace s knihovnou Raylib
target_link_libraries(PikeSquareSimulator PRIVATE raylib)

# Specifické systémové závislosti
if(NOT MSVC)
    target_compile_options(PikeSquareSimulator PRIVATE -Wall -Wextra)
endif()

if(UNIX AND NOT APPLE)
    # Na Linuxu musíme nalinkovat X11 a další grafické knihovny
    target_link_libraries(PikeSquareSimulator PRIVATE m pthread dl rt X11)
endif()