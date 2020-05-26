
#include <gtest/gtest.h>
#include <H5Cpp.h>
#include <util/netcdf_parser.h>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

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

    ASSERT_EQ(
            parser.time_info(),
            (NetCdfParser::NetCdfTimeInfo{
                    .time_start = boost::posix_time::to_time_t(boost::posix_time::ptime({1860, 1, 1}, {0, 0, 0})),
                    .time_unit = "days", // TODO: day/days/...
                    .delta = 1,
                    .delta_unit = "Year", // TODO: case?
                    .time_points = {51134, 54787},
            })
    );
}

auto get_source_dir() -> std::string {
    std::string file(__FILE__);
    auto last_slash = file.find_last_of('/');
    return file.substr(0, last_slash);
}

TEST(NetCdfParser, cSAR) {
    NetCdfParser parser(get_source_dir() + "/../data/cSAR_idiv_sm.nc");

    EXPECT_EQ(parser.ebv_class(), "Community composition");
    EXPECT_EQ(parser.ebv_name(), "Species diversity");
    EXPECT_EQ(parser.ebv_dataset(), "cSAR idiv");

    // TODO: segfault becaue file is broken
    // EXPECT_EQ(parser.ebv_subgroups(), (std::vector<std::string>{"scenario", "metric"}));

    //    // TODO: this value is broken
//    EXPECT_EQ(parser.ebv_subgroup_descriptions(),
//              std::vector<std::string>{"{Scenario: datasets produced under different SSP scenarios, metric:std metrics"});

    EXPECT_EQ(
            parser.ebv_subgroup_levels(),
            (std::vector<std::vector<std::string>>{
                    {"past_xx1.6", "past_xx2.0"},
                    {"mean",       "max"}
            })
    );

    EXPECT_EQ(
            parser.ebv_entity_levels(),
            (std::vector<std::string>{"0", "A", "F"})
    );

    EXPECT_EQ(
            parser.time_info(),
            (NetCdfParser::NetCdfTimeInfo{
                    .time_start = boost::posix_time::to_time_t(boost::posix_time::ptime({1860, 1, 1}, {0, 0, 0})),
                    .time_unit = "days", // TODO: day/days/...
                    .delta = 10,
                    .delta_unit = "Year", // TODO: case?
                    .time_points = {18262, 21914, 25567, 29219, 32872, 36524, 40177, 43829, 47482, 51134, 54787, 56613},
            })
    );
}

