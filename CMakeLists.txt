cmake_minimum_required(VERSION 3.20)

# ŘEŠENÍ PRO NOVÉ VERZE CMAKE (CLion 2025+):
# Nutí CMake považovat všechny závislosti (včetně Raylibu) za kompatibilní minimálně s verzí 3.5.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Aktivujeme politiku CMP0135 na úroveň NEW pro korektní extrakci timestampů
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

# Vypneme sestavení příkladů a her uvnitř Raylibu pro urychlení kompilace
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SUPPORT_GIF_RECORDING OFF CACHE BOOL "" FORCE)

# Stažení a konfigurace Raylibu
FetchContent_MakeAvailable(raylib)

# --- ZDROJOVÉ SOUBORY A HLAVIČKY ---
set(SOURCES
    src/main.cpp
    src/core/MathUtils.cpp
    src/formation/PikeSquare.cpp
    src/rendering/EngravingRenderer.cpp
    src/rendering/VectorRenderer.cpp
    src/ui/UIManager.cpp
)

set(HEADERS
    include/core/Types.h
    include/core/MathUtils.h
    include/simulation/Agent.h
    include/formation/PikeSquare.h
    include/rendering/IRenderer.h
    include/rendering/EngravingRenderer.h
    include/rendering/VectorRenderer.h
    include/ui/UIManager.h
)

# --- DEFINICE EXECUTABLE ---
add_executable(PikeSquareSimulator ${SOURCES} ${HEADERS})

# Registrace kořenového adresáře include/ pro naši aplikaci
target_include_directories(PikeSquareSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Označení hlaviček Raylibu jako SYSTEM pro potlačení cizích varování (např. raymath.h)
target_include_directories(PikeSquareSimulator SYSTEM PRIVATE
    $<TARGET_PROPERTY:raylib,INTERFACE_INCLUDE_DIRECTORIES>
)

# Propojení naší aplikace s knihovnou Raylib
target_link_libraries(PikeSquareSimulator PRIVATE raylib)

# Striktní varování pro náš kód (bez MSVC)
if(NOT MSVC)
    target_compile_options(PikeSquareSimulator PRIVATE -Wall -Wextra)
endif()

if(UNIX AND NOT APPLE)
    # Na Linuxu musíme nalinkovat X11 a další grafické knihovny
    target_link_libraries(PikeSquareSimulator PRIVATE m pthread dl rt X11)
endif()

# --- DEFINICE TEST EXECUTABLE (HEADLESS AUTOMATED TEST SUITE) ---
set(TEST_SOURCES
    tests/test_runner.cpp
    src/core/MathUtils.cpp
    src/formation/PikeSquare.cpp
    src/rendering/EngravingRenderer.cpp
    src/rendering/VectorRenderer.cpp
    src/ui/UIManager.cpp
)

add_executable(PikeSimTests ${TEST_SOURCES} ${HEADERS})

target_include_directories(PikeSimTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(PikeSimTests SYSTEM PRIVATE
    $<TARGET_PROPERTY:raylib,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(PikeSimTests PRIVATE raylib)

if(NOT MSVC)
    target_compile_options(PikeSimTests PRIVATE -Wall -Wextra)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(PikeSimTests PRIVATE m pthread dl rt X11)
endif()