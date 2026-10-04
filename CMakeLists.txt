cmake_minimum_required(VERSION 3.20)

project(
    brownturbo-discordrpc
    VERSION 0.0.1
    LANGUAGES C CXX
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
set(POLYHOOK_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(FORCE_STATIC_CRT OFF CACHE BOOL "" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(
    __BITSTREAM_NATIVE_END
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
)

add_library(${PROJECT_NAME} SHARED)

add_subdirectory(deps/rakhook/lib/minhook)
add_subdirectory(deps/rakhook)
add_subdirectory(deps/samp-api)

target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/samp_info.cpp
    src/discord_manager.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    deps/samp-api/include/sampapi
    deps/rakhook/lib/minhook/include
    deps/rakhook/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}"
    SUFFIX ".asi"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_NON_CONFORMING_SWPRINTFS
    _SILENCE_CXX23_ISSUES
    VALIDATE_OFFSETS=0
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /Zc:offsetof-
        /W3
        /wd4073
        /sdl-
        /Y-
    )
endif()

set(_DISCORD_RPC_BASE "${CMAKE_SOURCE_DIR}/vcpkg_installed/x86-windows")

add_library(discord-rpc::discord-rpc SHARED IMPORTED)
set_target_properties(discord-rpc::discord-rpc PROPERTIES
    IMPORTED_LOCATION         "${_DISCORD_RPC_BASE}/bin/discord-rpc.dll"
    IMPORTED_IMPLIB           "${_DISCORD_RPC_BASE}/lib/discord-rpc.lib"
    IMPORTED_LOCATION_DEBUG   "${_DISCORD_RPC_BASE}/debug/bin/discord-rpc.dll"
    IMPORTED_IMPLIB_DEBUG     "${_DISCORD_RPC_BASE}/debug/lib/discord-rpc.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_DISCORD_RPC_BASE}/include"
)

target_link_libraries(${PROJECT_NAME} PUBLIC minhook rakhook sampapi discord-rpc::discord-rpc)

function(force_dynamic_runtime_on_all_targets directory)
    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(subdirectory IN LISTS subdirectories)
        force_dynamic_runtime_on_all_targets("${subdirectory}")
    endforeach()

    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(target_type "${target}" TYPE)
        if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
            set_property(TARGET "${target}" PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
            )
        endif()
    endforeach()
endfunction()

force_dynamic_runtime_on_all_targets("${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DTRGT=${CMAKE_SOURCE_DIR}/vcpkg_installed/x86-windows/bin"
        "-DDEST=$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        -P "${CMAKE_SOURCE_DIR}/CopyRuntimeDlls.cmake"
    VERBATIM
    COMMENT "Copying runtime DLL dependencies for ${PROJECT_NAME}"
)
