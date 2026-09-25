# Patches mbedTLS' CMakeLists.txt (run as PATCH_COMMAND from the source dir).
#
# Geode builds Windows mods with clang using the GNU-style driver. mbedTLS
# detects the compiler via CMAKE_C_SIMULATE_ID (= MSVC) and then adds MSVC
# flags like /W3, which clang treats as file names. This makes mbedTLS use its
# clang settings whenever the compiler takes GNU-style arguments.

set(file "CMakeLists.txt")
set(marker "# GDHypeRate patch: GNU-style clang on Windows")
set(anchor "string(REGEX MATCH \"MSVC\" CMAKE_COMPILER_IS_MSVC \"\${COMPILER_ID}\")")

file(READ "${file}" content)

string(FIND "${content}" "${marker}" already_patched)
if (NOT already_patched EQUAL -1)
    return()
endif()

string(FIND "${content}" "${anchor}" anchor_pos)
if (anchor_pos EQUAL -1)
    message(FATAL_ERROR "patch-mbedtls: anchor not found, mbedTLS layout changed")
endif()

set(patch "${anchor}
${marker}
if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL \"GNU\")
    set(CMAKE_COMPILER_IS_MSVC \"\")
    string(REGEX MATCH \"Clang\" CMAKE_COMPILER_IS_CLANG \"\${CMAKE_C_COMPILER_ID}\")
endif()")

string(REPLACE "${anchor}" "${patch}" content "${content}")
file(WRITE "${file}" "${content}")
