if(NOT DEFINED TRGT)
    message(FATAL_ERROR "CopyRuntimeDlls.cmake: TRGT variable not provided")
endif()
if(NOT DEFINED DEST)
    message(FATAL_ERROR "CopyRuntimeDlls.cmake: DEST variable not provided")
endif()

if(DEST STREQUAL "")
    message(FATAL_ERROR "CopyRuntimeDlls.cmake: destination directory is empty")
endif()

if(TRGT STREQUAL "" OR TRGT STREQUAL "NOTFOUND")
    message(STATUS "CopyRuntimeDlls: no source directory provided - skipping.")
    return()
endif()

if(NOT EXISTS "${TRGT}")
    message(WARNING "CopyRuntimeDlls: source directory does not exist: ${TRGT}")
    return()
endif()

file(MAKE_DIRECTORY "${DEST}")

file(GLOB_RECURSE _runtime_files
    LIST_DIRECTORIES FALSE
    "${TRGT}/*.dll"
    "${TRGT}/*.pdb"
)

if(NOT _runtime_files)
    message(STATUS "CopyRuntimeDlls: no .dll/.pdb files found under ${TRGT} - skipping.")
    return()
endif()

foreach(_file IN LISTS _runtime_files)
    get_filename_component(_file_name "${_file}" NAME)
    message(STATUS "CopyRuntimeDlls: ${_file_name} -> ${DEST}")
    configure_file("${_file}" "${DEST}/${_file_name}" COPYONLY)
endforeach()
