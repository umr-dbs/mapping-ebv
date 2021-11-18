# BASE
add_library(mapping_ebv_base_lib OBJECT
        .
        )
target_include_directories(mapping_ebv_base_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mapping_ebv_base_lib PRIVATE ${MAPPING_CORE_PATH}/src)

# OPERATORS
add_library(mapping_ebv_operators_lib OBJECT
        .
        )
target_include_directories(mapping_ebv_operators_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mapping_ebv_operators_lib PRIVATE ${MAPPING_CORE_PATH}/src)

# SERVICES
add_library(mapping_ebv_services_lib OBJECT
        util/netcdf_parser.cpp
        services/geo_bon_catalog.cpp
        )
target_include_directories(mapping_ebv_services_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mapping_ebv_services_lib PRIVATE ${MAPPING_CORE_PATH}/src)

# DEPENDENCIES
target_include_directories(mapping_ebv_operators_lib PRIVATE ${Boost_INCLUDE_DIRS})

target_include_directories(mapping_ebv_base_lib PRIVATE ${jsoncpp_SOURCE_DIR}/include)
target_include_directories(mapping_ebv_operators_lib PRIVATE ${jsoncpp_SOURCE_DIR}/include)
target_include_directories(mapping_ebv_services_lib PRIVATE ${jsoncpp_SOURCE_DIR}/include)

target_include_directories(mapping_ebv_base_lib PRIVATE ${cpptoml_SOURCE_DIR}/include)
target_include_directories(mapping_ebv_operators_lib PRIVATE ${cpptoml_SOURCE_DIR}/include)
target_include_directories(mapping_ebv_services_lib PRIVATE ${cpptoml_SOURCE_DIR}/include)

target_include_directories(mapping_ebv_base_lib PRIVATE ${HDF5_CXX_INCLUDE_DIRS})
target_include_directories(mapping_ebv_operators_lib PRIVATE ${HDF5_CXX_INCLUDE_DIRS})
target_include_directories(mapping_ebv_services_lib PRIVATE ${HDF5_CXX_INCLUDE_DIRS})
