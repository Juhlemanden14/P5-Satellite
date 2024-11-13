# Collect all upper library directories.
file(
  GLOB P5-libs
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES true
  ${CMAKE_CURRENT_SOURCE_DIR}/*
)

# Add library directories
foreach(subdir ${P5-libs})
    # Check if the globbed file is a directory.
    if(IS_DIRECTORY ${subdir})
        # Check if the directory is a library by looking for a CMakelists.txt file.
        if(EXISTS ${subdir}/CMakeLists.txt)
            if (NOT TARGET plugin)
                add_subdirectory(${subdir} ${CMAKE_OUTPUT_DIRECTORY}/build/scratch/P5-Satellite/libraries/${subdir})
                message ("[P5-Builder] Added library: " ${subdir})
            endif()
        endif()
    endif()
endforeach()