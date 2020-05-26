
#ifndef MAPPING_EBV_NETCDF_PARSER_H
#define MAPPING_EBV_NETCDF_PARSER_H


#include <H5Cpp.h>
#include <string>
#include <vector>
#include <ostream>

class NetCdfParser {
    public:
        explicit NetCdfParser(const std::string &path) : file(H5::H5File(path, H5F_ACC_RDONLY)) {}

        // {"Conventions", "units", "creator", "description", "title", "ebv_class",
        //                                                                  "ebv_name", "ebv_dataset", "ebv_entity_levels", "EML", "_NCProperties",
        //                                                                  "ebv_subgroups", "ebv_subgroups_desc", "ebv_subgroups_levels",
        //                                                                  "ebv_subgroups_levels_desc", "ebv_entity_desc", "history",
        //                                                                  "ebv_entity_levels_desc"};

        auto ebv_class() const -> std::string;

        auto ebv_name() const -> std::string;

        auto ebv_dataset() const -> std::string;

        auto ebv_subgroups() const -> std::vector<std::string>;

        auto ebv_subgroup_descriptions() const -> std::vector<std::string>;

        auto ebv_subgroup_levels() const -> std::vector<std::vector<std::string>>;

        // TODO: ebv_subgroups_levels_desc

        auto ebv_entity_levels() const -> std::vector<std::string>;

        struct NetCdfTimeInfo {
            time_t time_start;
            std::string time_unit;

            int delta;
            std::string delta_unit;

            std::vector<double> time_points;

            friend std::ostream &operator<<(std::ostream &os, const NetCdfTimeInfo &info) {
                os << "time_start: " << info.time_start << " time_unit: " << info.time_unit << " delta: " << info.delta << " delta_unit: "
                   << info.delta_unit << " time_points: [";

                std::copy (info.time_points.cbegin(), info.time_points.cend(), std::ostream_iterator<double>(os, ", "));
                os << "\b\b]";

                return os;
            }

            bool operator==(const NetCdfTimeInfo &rhs) const {
                return time_start == rhs.time_start &&
                       time_unit == rhs.time_unit &&
                       delta == rhs.delta &&
                       delta_unit == rhs.delta_unit &&
                       time_points == rhs.time_points;
            }

            bool operator!=(const NetCdfTimeInfo &rhs) const {
                return !(rhs == *this);
            }
        };

        auto time_info() const -> NetCdfTimeInfo;

    private:
        H5::H5File file;
};

#endif //MAPPING_EBV_NETCDF_PARSER_H
