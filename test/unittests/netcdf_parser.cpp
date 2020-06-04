#include <gtest/gtest.h>
#include <util/netcdf_parser.h>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
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
                    .time_start = boost::posix_time::to_time_t(boost::posix_time::ptime({1860, 1, 1}, {0, 0, 0})),
                    .time_unit = "days", // TODO: day/days/...
                    .delta = 10,
                    .delta_unit = "Years", // TODO: case?
                    .time_points = {18262, 21914, 25567, 29219, 32872, 36524, 40177, 43829, 47482, 51134, 54787, 56613},
            })
    );
}
