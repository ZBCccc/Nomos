cmake_minimum_required(VERSION 3.14)

project(Nomos VERSION 1.0.0 LANGUAGES CXX C)

# --- Options ---
option(BUILD_TESTING "Build the testing tree." ON)

# --- Standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for better IntelliSense support in VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(HOMEBREW_PREFIX "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
else()
    set(HOMEBREW_PREFIX "/usr/local")
endif()

# Avoid hardcoding optional Homebrew keg-only paths (warns or breaks on machines
# that don't have that formula installed).
set(POSTGRESQL_LIB_DIR "${HOMEBREW_PREFIX}/lib/postgresql@14")
if(EXISTS "${POSTGRESQL_LIB_DIR}")
    link_directories("${POSTGRESQL_LIB_DIR}")
endif()

# include_directories(/usr/local/include)
# link_directories(/usr/local/lib)

# include_directories(${HOMEBREW_PREFIX}/include)
# link_directories(${HOMEBREW_PREFIX}/lib)

# --- Dependencies ---
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

find_path(GMP_INCLUDE_DIR gmp.h
    HINTS /usr/local/include /opt/homebrew/include
)
find_library(GMP_LIBRARY NAMES gmp libgmp
    HINTS /usr/local/lib /opt/homebrew/lib
)

if(NOT GMP_INCLUDE_DIR OR NOT GMP_LIBRARY)
    message(FATAL_ERROR "GMP not found. Set GMP_INCLUDE_DIR and GMP_LIBRARY if installed in a non-standard path.")
endif()

add_library(GMP::gmp UNKNOWN IMPORTED)
set_target_properties(GMP::gmp PROPERTIES
    IMPORTED_LOCATION "${GMP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
)

find_path(RELIC_INCLUDE_DIR relic/relic.h
    HINTS /usr/local/include /opt/homebrew/include
)
find_library(RELIC_LIBRARY NAMES relic relic_s librelic librelic_s
    HINTS /usr/local/lib /opt/homebrew/lib
)

if(NOT RELIC_INCLUDE_DIR OR NOT RELIC_LIBRARY)
    message(FATAL_ERROR
        "RELIC not found.\n"
        "  1) 通过 Homebrew 安装: brew install relic\n"
        "  2) 若安装在非标准路径，可指定：\n"
        "     cmake -DRELIC_INCLUDE_DIR=/path/to/relic/include -DRELIC_LIBRARY=/path/to/relic/lib/librelic.dylib .."
    )
endif()

add_library(RELIC::relic UNKNOWN IMPORTED)
set_target_properties(RELIC::relic PROPERTIES
    IMPORTED_LOCATION "${RELIC_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${RELIC_INCLUDE_DIR}"
)

# Ensure executables can load RELIC at runtime on macOS/Linux when RELIC's
# install_name is @rpath/librelic.* (common for Homebrew and custom builds).
get_filename_component(RELIC_LIBRARY_DIR "${RELIC_LIBRARY}" DIRECTORY)
if(EXISTS "${RELIC_LIBRARY_DIR}")
    list(APPEND CMAKE_BUILD_RPATH "${RELIC_LIBRARY_DIR}")
    list(APPEND CMAKE_INSTALL_RPATH "${RELIC_LIBRARY_DIR}")
    list(REMOVE_DUPLICATES CMAKE_BUILD_RPATH)
    list(REMOVE_DUPLICATES CMAKE_INSTALL_RPATH)
endif()
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# --- Include Directories (Global) ---
# Allows #include "nomos/Client.hpp" to work
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Subdirectories ---
# We will define the library and executable in src/CMakeLists.txt or here.
# For simplicity in this structure, I'll keep sources here but it's cleaner to separate.
# Let's use the current structure but define targets properly.

# --- Library Target ---
add_library(nomos_core
    src/nomos/Client.cpp
    src/nomos/Server.cpp
    src/nomos/Gatekeeper.cpp
    src/nomos/GatekeeperState.cpp
    src/mc-odxt/McOdxtExperiment.cpp
    src/core/Primitive.cpp
    src/core/CpABE.cpp


)

target_link_libraries(nomos_core PUBLIC
    OpenSSL::Crypto
    GMP::gmp
    RELIC::relic
    Threads::Threads
)

target_include_directories(nomos_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# --- Executable Target ---
add_executable(nomos_app src/main.cpp)
target_link_libraries(nomos_app PRIVATE nomos_core)
set_target_properties(nomos_app PROPERTIES OUTPUT_NAME "Nomos")

# --- Testing ---
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Formatting ---
# --- Formatting ---
find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    # Function to extract sources from targets and add a format target
    function(add_clang_format_target TARGET_NAME)
        set(SOURCES)
        foreach(T ${ARGN})
            get_target_property(T_SOURCES ${T} SOURCES)
            foreach(S ${T_SOURCES})
                get_filename_component(S_ABS ${S} ABSOLUTE)
                list(APPEND SOURCES ${S_ABS})
            endforeach()
        endforeach()
        
        # Filter duplicates
        list(REMOVE_DUPLICATES SOURCES)

        add_custom_target(${TARGET_NAME}
            COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${SOURCES}
            COMMENT "Formatting sources for ${TARGET_NAME}"
            VERBATIM
        )

        add_custom_target(check-${TARGET_NAME}
            COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -style=file ${SOURCES}
            COMMENT "Checking format for ${TARGET_NAME}"
            VERBATIM
        )
    endfunction()

    # Create global format targets for all main targets
    add_clang_format_target(format nomos_core nomos_app)
endif()
