# P5-Satellite

### Installing sns-3 properly [07-11-2024]
Download ns3.40
```
$ wget https://www.nsnam.org/releases/ns-3-40/documentation
```

cd into contrib/ directory
```
$ cd ns-allinone-3.40/ns-3.40/contrib/
```

Add sns3 and checkout on the working commit.
```
$ git clone https://github.com/sns3/sns3-satellite.git satellite
$ cd satellite
$ git checkout 9dc8f3522a690ea1358dd2ff784ccde567b0de33
```

Add traffic and checkout on the working commit.
```
$ git clone https://github.com/sns3/traffic.git traffic
$ cd traffic
$ git checkout b2ae5681968151e09f2fe5eb584d8eceed7215aa
```

Add magister-stats and checkout on the working commit.
```
$ git clone https://github.com/sns3/stats.git magister-stats
$ cd magister-stats
$ git checkout 5b3cc38f24d823be89acb945ac1cf5f55cbac7b2
```

Configure and build ns3.40
```
$ cd ..
$ ./ns3 configure $NS3_P5
$ ./ns3 build
```
After the build has finished run the tests
```
$ ./test.py
```

### Updating CMakeLists.txt to handle subdirectories
Make sure that you update your `scratch/CMakeLists.txt` file to be the one from `ns-3.40`, else ns-3 will not build the files as they are a subdirectory too deep.  
**The CMakeLists.txt from ns-3.40 can be copied here:**
``` 
set(target_prefix scratch_)

function(create_scratch source_files)
  # Return early if no sources in the subdirectory
  list(LENGTH source_files number_sources)
  if(number_sources EQUAL 0)
    return()
  endif()

  # If the scratch has more than a source file, we need to find the source with
  # the main function
  set(scratch_src)
  foreach(source_file ${source_files})
    file(READ ${source_file} source_file_contents)
    string(REGEX MATCHALL "main[(| (]" main_position "${source_file_contents}")
    if(CMAKE_MATCH_0)
      set(scratch_src ${source_file})
    endif()
  endforeach()

  if(NOT scratch_src)
    return()
  endif()

  # Get parent directory name
  get_filename_component(scratch_dirname ${scratch_src} DIRECTORY)
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" scratch_dirname
                 "${scratch_dirname}"
  )
  string(REPLACE "/" "_" scratch_dirname "${scratch_dirname}")

  # Get source name
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0")
    get_filename_component(scratch_name ${scratch_src} NAME_WLE)
  else()
    get_filename_component(scratch_name ${scratch_src} NAME)
    string(FIND "${scratch_name}" "." ext_position REVERSE)
    if(${ext_position} EQUAL -1)
      message(FATAL_ERROR "Source file has no extension: ${scratch_src}")
    else()
      string(SUBSTRING "${scratch_name}" 0 ${ext_position} scratch_name)
    endif()
  endif()

  set(target_prefix scratch_)
  if(scratch_dirname)
    # Join the names together if dirname is not the scratch folder
    set(target_prefix scratch${scratch_dirname}_)
  endif()

  # Get source absolute path and transform into relative path
  get_filename_component(scratch_src ${scratch_src} ABSOLUTE)
  get_filename_component(scratch_absolute_directory ${scratch_src} DIRECTORY)
  string(REPLACE "${PROJECT_SOURCE_DIR}" "${CMAKE_OUTPUT_DIRECTORY}"
                 scratch_directory ${scratch_absolute_directory}
  )
  build_exec(
          EXECNAME ${scratch_name}
          EXECNAME_PREFIX ${target_prefix}
          SOURCE_FILES "${source_files}"
          LIBRARIES_TO_LINK "${ns3-libs}" "${ns3-contrib-libs}"
          EXECUTABLE_DIRECTORY_PATH ${scratch_directory}/
  )
endfunction()

# Scan *.cc files in ns-3-dev/scratch and build a target for each
file(GLOB single_source_file_scratches CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.cc)
foreach(scratch_src ${single_source_file_scratches})
  create_scratch(${scratch_src})
endforeach()

# Scan *.cc files in ns-3-dev/scratch subdirectories and build a target for each
# subdirectory
file(
  GLOB_RECURSE scratch_subdirectories
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES true
  ${CMAKE_CURRENT_SOURCE_DIR}/**
)
# Filter out files
foreach(entry ${scratch_subdirectories})
  if(NOT (IS_DIRECTORY ${entry}))
    list(REMOVE_ITEM scratch_subdirectories ${entry})
  endif()
endforeach()

foreach(subdir ${scratch_subdirectories})
  if(EXISTS ${subdir}/CMakeLists.txt)
    # If the subdirectory contains a CMakeLists.txt file
    # we let the CMake file manage the source files
    #
    # Use this if you want to link to external libraries
    # without creating a module
    add_subdirectory(${subdir})
  else()
    # Otherwise we pick all the files in the subdirectory
    # and create a scratch for them automatically
    file(GLOB scratch_sources CONFIGURE_DEPENDS ${subdir}/[^.]*.cc)
    create_scratch("${scratch_sources}")
  endif()
endforeach()
```
