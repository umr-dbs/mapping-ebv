#include <memory>
#include <util/concat.h>
#include <util/timeparser.h>
#include <algorithm>
#include <util/log.h>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include "netcdf_parser.h"
#include <gdal/ogr_spatialref.h>

auto attribute_to_string(const H5::Attribute &attribute) -> std::string {
    std::string buffer;
    attribute.read(attribute.getDataType(), buffer);
    return buffer;
}

auto attribute_to_string_vector(const H5::Attribute &attribute) -> std::vector<std::string> {
    const auto number_of_values = attribute.getSpace().getSimpleExtentNpoints();

    using char_vec = std::vector<char *>;
    const std::unique_ptr<char_vec, void (*)(char_vec *)> buffer(
            new char_vec(number_of_values),
            // delete char pointer to avoid leaking...
            [](char_vec *vec) {
                for (const auto &v : *vec) {
                    delete v;
                }
            }
    );

    attribute.read(attribute.getDataType(), (void *) buffer->data());

    std::vector<std::string> strings(number_of_values);

    for (auto i = 0; i < number_of_values; ++i) {
        strings[i] = (*buffer)[i];
    }

    return strings;
}

auto dataset_to_string_vector(const H5::DataSet &dataSet) -> std::vector<std::string> {
    const auto number_of_values = dataSet.getSpace().getSimpleExtentNpoints();

    using char_vec = std::vector<char *>;
    const std::unique_ptr<char_vec, void (*)(char_vec *)> buffer(
            new char_vec(number_of_values),
            // delete char pointer to avoid leaking...
            [](char_vec *vec) {
                for (const auto &v : *vec) {
                    delete v;
                }
            }
    );

    dataSet.read((void *) buffer->data(), dataSet.getDataType());

    std::vector<std::string> strings(number_of_values);

    for (auto i = 0; i < number_of_values; ++i) {
        strings[i] = (*buffer)[i];
    }

    return strings;
}

auto dataset_to_float_vector(const H5::DataSet &dataSet) -> std::vector<float> {
    const auto number_of_values = dataSet.getSpace().getSimpleExtentNpoints();

    std::vector<float> buffer(number_of_values);

    dataSet.read(static_cast<void *>(buffer.data()), dataSet.getDataType());

    return buffer;
}

auto NetCdfParser::crs_wkt() const -> std::string {
    const auto dataSet = file.openDataSet("crs");
    const auto attribute = dataSet.openAttribute("spatial_ref");
    return attribute_to_string(attribute);
}

auto NetCdfParser::crs_as_code() const -> std::string {

    const std::string crs_wkt = this->crs_wkt();
    Log::debug(concat("NetCdfParser: CRS wkt string: ", crs_wkt));

    std::string crs_code = "";
    OGRSpatialReference sref = OGRSpatialReference(crs_wkt.c_str());
    if (sref.IsGeographic()) {
        const std::string geogcs_authority = std::string(sref.GetAuthorityName("GEOGCS"));
        const std::string geogcs_code = std::string(sref.GetAuthorityCode("GEOGCS"));
        crs_code = concat(geogcs_authority, ":", geogcs_code);
    }
    if (sref.IsProjected()) {
        const std::string projcs_authority = std::string(sref.GetAuthorityName("PROJCS"));
        const std::string projcs_code = std::string(sref.GetAuthorityCode("PROJCS"));
        crs_code = concat(projcs_authority, ":", projcs_code);
    }
    
    Log::debug(concat("NetCdfParser: CRS code: ", crs_code));

    return crs_code;
}

auto NetCdfParser::ebv_class() const -> std::string {
    const auto attribute = file.openAttribute("ebv_class");
    return attribute_to_string(attribute);
}

auto NetCdfParser::ebv_name() const -> std::string {
    const auto attribute = file.openAttribute("ebv_name");
    return attribute_to_string(attribute);
}

auto NetCdfParser::ebv_dataset() const -> std::string {
    const auto attribute = file.openAttribute("ebv_dataset");
    return attribute_to_string(attribute);
}

auto NetCdfParser::ebv_subgroups() const -> std::vector<std::string> {
    const auto attribute = file.openAttribute("ebv_subgroups");

    return attribute_to_string_vector(attribute);
}

auto NetCdfParser::ebv_subgroup_descriptions() const -> std::vector<std::string> {
    const auto attribute = file.openAttribute("ebv_subgroups_desc");

    return attribute_to_string_vector(attribute);
}

template<class T>
auto read_attribute_optionally(const T &group, const std::string &field, const std::string &default_value) -> std::string {
    if (group.attrExists(field)) {
        return attribute_to_string(group.openAttribute(field));
    } else {
        return default_value;
    }
}

/// Parses ebv subgroup levels
///
/// Assumes that the attribute is non-empty
auto
NetCdfParser::ebv_subgroup_values(const std::string &subgroup_name,
                                  const std::vector<std::string> &path) const -> std::vector<NetCdfValue> {
    const auto attribute = file.openAttribute(concat("ebv_var_", subgroup_name));

    std::vector<std::string> values;
    if (subgroup_name == "entity") { // special treatment for entities
        const auto pointer_to_variable = attribute_to_string(attribute);
        const auto dataset = file.openDataSet(pointer_to_variable);

        values = dataset_to_string_vector(dataset);
    } else {
        values = attribute_to_string_vector(attribute);
    }

    std::vector<NetCdfValue> result;
    result.reserve(values.size());

    // open groups from `path`
    H5::Group group = file.openGroup("/"); // open root group
    for (const auto &group_name : path) {
        group = group.openGroup(group_name);
    }

    for (const auto &value : values) {
        try {
            std::string label;
            std::string description;

            if (subgroup_name == "entity") { // special treatment for entities
                const auto dataset = group.openDataSet(value);

                label = read_attribute_optionally(dataset, "label", value);
                description = read_attribute_optionally(dataset, "description", "");
            } else {
                const auto subgroup = group.openGroup(value);

                label = read_attribute_optionally(subgroup, "label", value);
                description = read_attribute_optionally(subgroup, "description", "");
            }

            result.push_back(NetCdfValue{
                    .name = value,
                    .label = label,
                    .description = description
            });
        } catch (const H5::Exception &e) {
            Log::debug("Unable to open group or dataset `%s` in file `%s` (%s)",
                       value.c_str(), file.getFileName().c_str(), e.getDetailMsg().c_str());
        }
    }

    return result;
}

auto NetCdfParser::time_info() const -> NetCdfParser::NetCdfTimeInfo {
    const auto time_field = file.openDataSet("time");

    const auto time_reference_raw_string = attribute_to_string(time_field.openAttribute("units"));

    const auto time_reference_split_position = time_reference_raw_string.find(" since ");

    auto time_reference_unit = time_reference_raw_string.substr(0, time_reference_split_position);
    std::transform(time_reference_unit.begin(), time_reference_unit.end(), time_reference_unit.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    const auto time_reference_string = time_reference_raw_string.substr(time_reference_split_position + sizeof(" since ") - 1,
                                                                        time_reference_raw_string.length());

    const auto time_start = TimeParser::createCustom("%Y-%m-%d  %H:%M:%S")->parse(time_reference_string);

    const auto time_delta_raw_string = attribute_to_string(time_field.openAttribute("t_delta"));

    const auto time_delta_space_position = time_delta_raw_string.find(' ');
    const auto time_delta_string = time_delta_raw_string.substr(0, time_delta_space_position);
    auto time_delta_unit = time_delta_raw_string.substr(time_delta_space_position + 1, time_delta_raw_string.length());
    std::transform(time_delta_unit.begin(), time_delta_unit.end(), time_delta_unit.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    const auto time_vector = dataset_to_float_vector(time_field);
    const auto time_points = std::vector<double>(time_vector.cbegin(), time_vector.cend());

    return {
            .time_start = time_start,
            .time_unit = time_reference_unit,
            .delta = static_cast<int>(strtol(time_delta_string.c_str(), nullptr, 10)),
            .delta_unit = time_delta_unit,
            .time_points_unix = NetCdfParser::time_points_as_unix(time_start, time_reference_unit, time_points),
            .time_points = time_points,
    };
}

auto NetCdfParser::time_points_as_unix(double time_start,
                                       const std::string &time_unit,
                                       const std::vector<double> &time_points) -> std::vector<double> {
    std::vector<double> unix_time_points;
    unix_time_points.reserve(time_points.size());

    if (time_unit == "days") {
        const auto seconds_per_day = 24 * 60 * 60;

        for (const double time_point_raw : time_points) {
            const double time_point = time_start + time_point_raw * seconds_per_day;
            unix_time_points.emplace_back(time_point);
        }
    } else {
        // TODO: boost calendar additions
        const boost::posix_time::ptime posix_time_start = boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1)) +
                                                          boost::posix_time::milliseconds(static_cast<long>(time_start * 1000));
    }

    return unix_time_points;
}

bool NetCdfParser::NetCdfValue::operator==(const NetCdfParser::NetCdfValue &rhs) const {
    return name == rhs.name &&
           label == rhs.label &&
           description == rhs.description;
}

bool NetCdfParser::NetCdfValue::operator!=(const NetCdfParser::NetCdfValue &rhs) const {
    return !(rhs == *this);
}

auto NetCdfParser::NetCdfValue::to_json() const -> Json::Value {
    Json::Value json(Json::objectValue);
    json["name"] = this->name;
    json["label"] = this->label;
    json["description"] = this->description;
    return json;
}

std::ostream &operator<<(std::ostream &os, const NetCdfParser::NetCdfValue &value) {
    os << "name: " << value.name << " label: " << value.label << " description: " << value.description;
    return os;
}
