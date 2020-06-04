#include <memory>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <util/concat.h>
#include "netcdf_parser.h"

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

/// Parses ebv subgroup levels
///
/// Assumes that the attribute is non-empty
auto
NetCdfParser::ebv_subgroup_values(const std::string &subgroup_name,
                                  const std::vector<std::string> &path) const -> std::vector<NetCdfValue> {
    const auto attribute = file.openAttribute(concat("ebv_var_", subgroup_name));

    std::vector<std::string> values;
    if (subgroup_name == "entity") { // special treatment for entities
        if (path.empty()) throw NetCdfParserException("NetCdfParserException: Empty subgroup path");

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
        std::string label;
        std::string description;

        if (subgroup_name == "entity") { // special treatment for entities
            const auto dataset = group.openDataSet(value);
            label = attribute_to_string(dataset.openAttribute("label"));
            description = attribute_to_string(dataset.openAttribute("description"));
        } else {
            const auto subgroup = group.openGroup(value);
            label = attribute_to_string(subgroup.openAttribute("label"));
            description = attribute_to_string(subgroup.openAttribute("description"));
        }

        result.push_back(NetCdfValue{
                .name = value,
                .label = label,
                .description = description
        });
    }

    return result;
}

auto NetCdfParser::time_info() const -> NetCdfParser::NetCdfTimeInfo {
    const auto time_field = file.openDataSet("time");

    const auto time_reference_raw_string = attribute_to_string(time_field.openAttribute("units"));

    const auto time_reference_split_position = time_reference_raw_string.find(" since ");
    const auto time_reference_unit = time_reference_raw_string.substr(0, time_reference_split_position);
    const auto time_reference_string = time_reference_raw_string.substr(time_reference_split_position + sizeof(" since ") - 1,
                                                                        time_reference_raw_string.length());

    const auto time_start = boost::posix_time::time_from_string(time_reference_string);

    const auto time_delta_raw_string = attribute_to_string(time_field.openAttribute("t_delta"));

    const auto time_delta_space_position = time_delta_raw_string.find(' ');
    const auto time_delta_string = time_delta_raw_string.substr(0, time_delta_space_position);
    const auto time_delta_unit = time_delta_raw_string.substr(time_delta_space_position + 1, time_delta_raw_string.length());

    const auto time_vector = dataset_to_float_vector(time_field);

    return {
            .time_start = boost::posix_time::to_time_t(time_start),
            .time_unit = time_reference_unit,
            .delta = static_cast<int>(strtol(time_delta_string.c_str(), nullptr, 10)),
            .delta_unit = time_delta_unit,
            .time_points = std::vector<double>(time_vector.cbegin(), time_vector.cend()),
    };
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
