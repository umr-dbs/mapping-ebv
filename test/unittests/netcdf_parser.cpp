#include <gtest/gtest.h>
#include <util/netcdf_parser.h>
#include <util/timeparser.h>
#include "util.h"

TEST(NetCdfParser, cSAR) { // NOLINT(cert-err58-cpp)
    NetCdfParser parser(test_util::get_data_dir() + "1/netcdf/cSAR_idiv_v1.nc");

    EXPECT_EQ(parser.ebv_class(), "Community composition");
    EXPECT_EQ(parser.ebv_name(), "Species diversity");
    EXPECT_EQ(parser.ebv_dataset(), "cSAR idiv");

    EXPECT_EQ(parser.ebv_subgroups(), (std::vector<std::string>{"scenario", "metric", "entity"}));

    EXPECT_EQ(parser.ebv_subgroup_descriptions(),
              (std::vector<std::string>{"one scenario", "one metric", "three entities: non forest birds, forest birds, all birds"})
    );

    EXPECT_EQ(
            parser.ebv_subgroup_values("scenario", {}),
            (std::vector<NetCdfParser::NetCdfValue>{
                    {.name = "past", .label= "past: 1900 - 2015", .description="calculations where done per decade betrween 1900 and 2015"},
            })
    );

    EXPECT_EQ(
            parser.ebv_subgroup_values("metric", std::vector<std::string>{"past"}),
            (std::vector<NetCdfParser::NetCdfValue>{
                    {.name = "mean", .label= "mean", .description="mean values per decade"},
            })
    );

    EXPECT_EQ(
            parser.ebv_subgroup_values("entity", std::vector<std::string>{"past", "mean"}),
            (std::vector<NetCdfParser::NetCdfValue>{
                    {
                            .name = "0",
                            .label= "non forest birds species",
                            .description="Changes in bird diversity at the grid cell level caused by land-use, estimated by the cSAR model "
                                         "(Martins & Pereira, 2017). It reports changes in species number (percentage and absolute), "
                                         "relative to 1900, for all bird species, forest bird species, and non-forest bird species in each "
                                         "cell. Uses the LUH 2.0 projections for land-use, and the PREDICTS coefficients for bird "
                                         "affinities to land-uses."
                    },
                    {
                            .name = "A",
                            .label= "all brid species",
                            .description="Changes in bird diversity at the grid cell level caused by land-use, estimated by the cSAR model "
                                         "(Martins & Pereira, 2017). It reports changes in species number (percentage and absolute), "
                                         "relative to 1900, for all bird species, forest bird species, and non-forest bird species in "
                                         "each cell. Uses the LUH 2.0 projections for land-use, and the PREDICTS coefficients for bird "
                                         "affinities to land-uses."
                    },
                    {
                            .name = "F",
                            .label= "forest bird species",
                            .description="Changes in bird diversity at the grid cell level caused by land-use, estimated by the cSAR model "
                                         "(Martins & Pereira, 2017). It reports changes in species number (percentage and absolute), "
                                         "relative to 1900, for all bird species, forest bird species, and non-forest bird species in each "
                                         "cell. Uses the LUH 2.0 projections for land-use, and the PREDICTS coefficients for bird "
                                         "affinities to land-uses."
                    }
            })
    );

    EXPECT_EQ(
            parser.time_info(),
            (NetCdfParser::NetCdfTimeInfo{
                    .time_start = TimeParser::createCustom("%Y-%m-%d")->parse("1860-01-01"),
                    .time_unit = "days",
                    .delta = 10,
                    .delta_unit = "years",
                    .time_points_unix = {-1893456000, -1577923200, -1262304000, -946771200, -631152000, -315619200,
                                         0, 315532800, 631152000, 946684800, 1262304000, 1420070400},
                    .time_points = {18262, 21914, 25567, 29219, 32872, 36524, 40177, 43829, 47482, 51134, 54787, 56613},
            })
    );

    EXPECT_EQ(
            parser.crs_as_code(),
            "EPSG:4326"
    );

    const auto unit_range = parser.unit_range(std::string{"past/mean/0"});
    ASSERT_EQ(unit_range.size(), 2);
    EXPECT_DOUBLE_EQ(unit_range[0], -31.24603271484375);
    EXPECT_DOUBLE_EQ(unit_range[1], 31.14495849609375);
}
