cmake_minimum_required(VERSION 3.18)
project(MyProject)

# Function to extract version and Git info from a given directory
# Usage: extract_version_and_git_info(<path_to_directory_containing_vcpkg.json>)
function(repo_info passedDirectory)
    # --- Read version from vcpkg.json ---
    set(VCPKG_JSON_PATH "${passedDirectory}/vcpkg.json")
    if (EXISTS "${VCPKG_JSON_PATH}")
        file(READ "${VCPKG_JSON_PATH}" VCPKG_JSON_CONTENT)

        # Parse version (CMake 3.19+ has string(JSON), fallback to regex)

        string(JSON VERSION_STRING GET "${VCPKG_JSON_CONTENT}" "version-string")

        # Split version into MAJOR/MINOR/PATCH
        if (VERSION_STRING MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
            set(VERSION_MAJOR ${CMAKE_MATCH_1} PARENT_SCOPE)
            set(VERSION_MINOR ${CMAKE_MATCH_2} PARENT_SCOPE)
            set(VERSION_PATCH ${CMAKE_MATCH_3} PARENT_SCOPE)
            message(STATUS "Version: ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
        else ()
            message(WARNING "Invalid version format in vcpkg.json. Using 0.0.0")
            set(VERSION_MAJOR "0" PARENT_SCOPE)
            set(VERSION_MINOR "0" PARENT_SCOPE)
            set(VERSION_PATCH "0" PARENT_SCOPE)
        endif ()
    else ()
        message(WARNING "vcpkg.json not found in ${passedDirectory}. Using 0.0.0")
        set(VERSION_MAJOR "0" PARENT_SCOPE)
        set(VERSION_MINOR "0" PARENT_SCOPE)
        set(VERSION_PATCH "0" PARENT_SCOPE)
    endif ()

    # --- Get Git info (commit hash and branch) ---
    find_package(Git QUIET)
    if (GIT_FOUND)
        # Get commit hash (short)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                WORKING_DIRECTORY ${passedDirectory}
                OUTPUT_VARIABLE GIT_COMMIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
        )
        # Get branch name
        execute_process(
                COMMAND ${GIT_EXECUTABLE} branch --show-current
                WORKING_DIRECTORY ${passedDirectory}
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
        )
    endif ()

    # Set defaults if Git failed
    if (NOT GIT_COMMIT_HASH)
        set(GIT_COMMIT_HASH "unknown")
    endif ()
    if (NOT GIT_BRANCH)
        set(GIT_BRANCH "unknown")
    endif ()

    # Export Git variables
    set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH} PARENT_SCOPE)
    set(GIT_BRANCH ${GIT_BRANCH} PARENT_SCOPE)
    message(STATUS "Git commit: ${GIT_COMMIT_HASH}")
    message(STATUS "Git branch: ${GIT_BRANCH}")
endfunction()
