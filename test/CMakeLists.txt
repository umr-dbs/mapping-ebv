add_library(mapping_ebv_unittests_lib OBJECT
        unittests/netcdf_parser.cpp
        unittests/netcdf_tests.cpp
        )
target_include_directories(mapping_ebv_unittests_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_include_directories(mapping_ebv_unittests_lib PRIVATE ${MAPPING_CORE_PATH}/src)

target_include_directories(mapping_ebv_unittests_lib PRIVATE ${jsoncpp_SOURCE_DIR}/include)
target_include_directories(mapping_ebv_unittests_lib PRIVATE ${cpptoml_SOURCE_DIR}/include)
target_include_directories(mapping_ebv_unittests_lib PRIVATE ${HDF5_CXX_INCLUDE_DIRS})

# Add unit test library in standalone mode
if (NOT is_mapping_module)
    add_executable(mapping_ebv_unittests EXCLUDE_FROM_ALL unittests/init.cpp)

    target_link_libraries_internal(mapping_ebv_unittests mapping_ebv_unittests_lib)
#    target_link_libraries_internal(mapping_ebv_unittests mapping_ebv_services_lib)

#    target_link_libraries_internal(mapping_ebv_unittests_lib mapping_core_services_lib)

    target_link_libraries_internal(mapping_ebv_unittests jsoncpp_lib_static)

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

    target_link_libraries(mapping_ebv_unittests_lib gtest ${HDF5_CXX_LIBRARIES} ${Boost_LIBRARIES})
endif (NOT is_mapping_module)

set(systemtests ${systemtests} PARENT_SCOPE)
