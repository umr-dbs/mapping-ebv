
#ifndef MAPPING_EBV_NETCDF_PARSER_H
#define MAPPING_EBV_NETCDF_PARSER_H


#include <H5Cpp.h>
#include <string>
#include <vector>
#include <ostream>
#include <iterator>
#include <json/json.h>

class NetCdfParser {
    public:
        struct NetCdfValue {
            bool operator==(const NetCdfValue &rhs) const;

            bool operator!=(const NetCdfValue &rhs) const;

            friend std::ostream &operator<<(std::ostream &os, const NetCdfValue &value);

            auto to_json() const -> Json::Value;

            std::string name;
            std::string label;
            std::string description;
        };

        struct NetCdfParserException : public std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        explicit NetCdfParser(const std::string &path) : file(H5::H5File(path, H5F_ACC_RDONLY)) {}

        auto crs_wkt() const -> std::string;

        auto crs_as_code() const -> std::string;
        
        auto ebv_class() const -> std::string;

        auto ebv_name() const -> std::string;

        auto ebv_dataset() const -> std::string;

        auto ebv_subgroups() const -> std::vector<std::string>;

        auto ebv_subgroup_descriptions() const -> std::vector<std::string>;

        auto ebv_subgroup_values(const std::string &subgroup_name, const std::vector<std::string> &path) const -> std::vector<NetCdfValue>;

        struct NetCdfTimeInfo {
            double time_start;
            std::string time_unit;

            int delta;
            std::string delta_unit;

            std::vector<double> time_points_unix;
            std::vector<double> time_points;

            friend std::ostream &operator<<(std::ostream &os, const NetCdfTimeInfo &info) {
                os << "time_start: " << info.time_start << " time_unit: " << info.time_unit << " delta: " << info.delta << " delta_unit: "
                   << info.delta_unit << " time_points: [";

                std::copy(info.time_points.cbegin(), info.time_points.cend(), std::ostream_iterator<double>(os, ", "));
                os << "\b\b] time_points_unix: [";

                std::copy(info.time_points_unix.cbegin(), info.time_points_unix.cend(), std::ostream_iterator<double>(os, ", "));
                os << "\b\b]";

                return os;
            }

            bool operator==(const NetCdfTimeInfo &rhs) const {
                return time_start == rhs.time_start &&
                       time_unit == rhs.time_unit &&
                       delta == rhs.delta &&
                       delta_unit == rhs.delta_unit &&
                       time_points == rhs.time_points &&
                       time_points_unix == rhs.time_points_unix;
            }

            bool operator!=(const NetCdfTimeInfo &rhs) const {
                return !(rhs == *this);
            }
        };

        auto time_info() const -> NetCdfTimeInfo;

    protected:
        static auto time_points_as_unix(double time_start,
                                 const std::string &time_unit,
                                 const std::vector<double> &time_points) -> std::vector<double>;

    private:
        H5::H5File file;
};

#endif //MAPPING_EBV_NETCDF_PARSER_H
