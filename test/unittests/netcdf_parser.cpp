
#include <gtest/gtest.h>
#include <H5Cpp.h>
#include <util/netcdf_parser.h>

TEST(NetCdfParser, base) {
    H5::H5File file("/home/beilschmidt/CLionProjects/mapping-ebv/mamals_ncd_subgroups_times.nc", H5F_ACC_RDONLY);

    const auto number_of_objects = 9;
    ASSERT_EQ(file.getNumObjs(), number_of_objects);

    std::string object_names[number_of_objects];
    for (auto i = 0; i < file.getNumObjs(); ++i) {
        object_names[i] = file.getObjnameByIdx(i);
    }

    std::string expected_object_names[number_of_objects] = {"SSP1xRCP2.6", "SSP3xRCP6.0", "SSP5xRCP8.5", "dim_entity", "lat", "lon",
                                                            "spatial_ref", "time", "var_entities"};

    for (auto i = 0; i < number_of_objects; ++i) {
        EXPECT_EQ(object_names[i], expected_object_names[i]);
    }

    const auto dataset = file.openDataSet("var_entities");

    ASSERT_EQ(dataset.getNumAttrs(), 1);

    const auto entities_attribute = dataset.openAttribute((const unsigned int) 0);
    ASSERT_EQ(entities_attribute.getName(), "DIMENSION_LIST");

    const auto number_of_attributes = 18;
    ASSERT_EQ(file.getNumAttrs(), number_of_attributes);

    std::string attribute_names[number_of_attributes];
    for (auto i = 0; i < number_of_attributes; ++i) {
        attribute_names[i] = file.openAttribute(i).getName();
    }

    std::string expected_attribute_names[number_of_attributes] = {"Conventions", "units", "creator", "description", "title", "ebv_class",
                                                                  "ebv_name", "ebv_dataset", "ebv_entity_levels", "EML", "_NCProperties",
                                                                  "ebv_subgroups", "ebv_subgroups_desc", "ebv_subgroups_levels",
                                                                  "ebv_subgroups_levels_desc", "ebv_entity_desc", "history",
                                                                  "ebv_entity_levels_desc"};

    for (auto i = 0; i < number_of_attributes; ++i) {
        EXPECT_EQ(attribute_names[i], expected_attribute_names[i]);
    }

    const auto subgroups_attribute = file.openAttribute("ebv_subgroups");
    ASSERT_EQ(subgroups_attribute.getDataType().getClass(), H5T_STRING);

    std::string buffer;
    subgroups_attribute.read(subgroups_attribute.getDataType(), buffer);
    ASSERT_EQ(buffer, "scenario");

    const auto subgroups_levels_attribute = file.openAttribute("ebv_subgroups_levels");
    ASSERT_EQ(subgroups_levels_attribute.getDataType().getClass(), H5T_STRING);

    std::string buffer2;
    subgroups_levels_attribute.read(subgroups_levels_attribute.getDataType(), buffer2);
    ASSERT_EQ(buffer2, "SSP1xRCP2.6,SSP3xRCP6.0,SSP5xRCP8.5;sum,mean,median");
    ASSERT_EQ(subgroups_levels_attribute.getSpace().getSelectNpoints(), 1);
}

TEST(NetCdfParser, attributes) {
    // TODO: store example files
    NetCdfParser parser("../../mamals_ncd_subgroups_times.nc");

    ASSERT_EQ(parser.ebv_class(), "Species Distribution");
    ASSERT_EQ(parser.ebv_name(), "IUCN range maps");
    ASSERT_EQ(parser.ebv_dataset(), "IUCN Mammals");

    ASSERT_EQ(parser.ebv_subgroups(), (std::vector<std::string>{"scenario", "metric"}));
    // TODO: this value is broken
    ASSERT_EQ(parser.ebv_subgroup_descriptions(),
              std::vector<std::string>{"{Scenario: datasets produced under different SSP scenarios, metric:std metrics"});

    ASSERT_EQ(
            parser.ebv_subgroup_levels(),
            (std::vector<std::vector<std::string>>{
                    {"SSP1xRCP2.6", "SSP3xRCP6.0", "SSP5xRCP8.5"},
                    {"sum",         "mean",        "median"}
            })
    );

    ASSERT_EQ(
            parser.ebv_entity_levels(),
            (std::vector<std::string>{"Abditomys_latidens", "Crocidura_pachyura", "Hylaeamys_perenensis", "Mydaus_marchei",
                                      "Praomys_daltoni", "Tadarida_lobata"})
    );
}
