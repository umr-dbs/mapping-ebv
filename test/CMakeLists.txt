add_library(mapping_gfbio_unittests_lib OBJECT
        unittests/netcdf_parser.cpp
        )

target_include_directories(mapping_gfbio_unittests_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_include_directories(mapping_gfbio_unittests_lib PRIVATE ${MAPPING_CORE_PATH}/src)

target_include_directories(mapping_gfbio_unittests_lib PRIVATE ${jsoncpp_SOURCE_DIR}/include)
target_include_directories(mapping_gfbio_unittests_lib PRIVATE ${cpptoml_SOURCE_DIR}/include)
target_include_directories(mapping_gfbio_unittests_lib PRIVATE ${HDF5_CXX_INCLUDE_DIRS})

# Add unit test library in standalone mode
if (NOT is_mapping_module)
    add_executable(mapping_gfbio_unittests EXCLUDE_FROM_ALL unittests/init.cpp)

    target_link_libraries_internal(mapping_gfbio_unittests mapping_gfbio_unittests_lib)
    target_link_libraries_internal(mapping_gfbio_unittests mapping_gfbio_services_lib)

    download_project(PROJ       googletest
            GIT_REPOSITORY      https://github.com/google/googletest.git
            GIT_TAG             release-1.8.0
            UPDATE_DISCONNECTED 1
            PREFIX CMakeFiles/Download
            )
    # Prevent GoogleTest from overriding our compiler/linker options
    # when building with Visual Studio
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)

    target_link_libraries(mapping_gfbio_unittests_lib gtest ${HDF5_CXX_LIBRARIES})
endif (NOT is_mapping_module)

#list(APPEND systemtests terminology_resolver_first)
set(systemtests ${systemtests} PARENT_SCOPE)
