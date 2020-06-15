#include <gtest/gtest.h>
#include <util/netcdf_parser.h>
#include <util/timeparser.h>
#include "util.h"

TEST(NetCdfParser, cSAR) { // NOLINT(cert-err58-cpp)
    NetCdfParser parser(test_util::get_data_dir() + "cSAR_idiv_004.nc");

    EXPECT_EQ(parser.ebv_class(), "Community composition");
    EXPECT_EQ(parser.ebv_name(), "Species diversity");
    EXPECT_EQ(parser.ebv_dataset(), "cSAR idiv");

    EXPECT_EQ(parser.ebv_subgroups(), (std::vector<std::string>{"scenario", "metric", "entity"}));

    EXPECT_EQ(parser.ebv_subgroup_descriptions(),
              (std::vector<std::string>{"two different scenarions where used - past_xx1, past_xx2", "used metrics are mean and max",
                                        "different grouping of bird species"})
    );

    EXPECT_EQ(
            parser.ebv_subgroup_values("scenario", {}),
            (std::vector<NetCdfParser::NetCdfValue>{
                    {.name = "past_xx1", .label= "past not condidering CO2", .description="some discription for past not condidering CO2"},
                    {.name = "past_xx2", .label= "past condidering CO2", .description="some discription for past condidering CO2"}
            })
    );

    EXPECT_EQ(
            parser.ebv_subgroup_values("metric", std::vector<std::string>{"past_xx1"}),
            (std::vector<NetCdfParser::NetCdfValue>{
                    {.name = "mean", .label= "mean values per decade", .description="some discription for mean values per decade"},
                    {.name = "max", .label= "max value per decade", .description="some discription for max value per decade"}
            })
    );

    EXPECT_EQ(
            parser.ebv_subgroup_values("entity", std::vector<std::string>{"past_xx1", "mean"}),
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
}
