#include <memory>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
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

auto attribute_to_2d_string_vector(const H5::Attribute &attribute) -> std::vector<std::vector<std::string>> {
    const auto number_of_values = attribute.getSpace().getSimpleExtentNpoints();

    fprintf(stderr, "num: %lld \n", number_of_values);

    hsize_t dims_out[2];
    int ndims = attribute.getSpace().getSimpleExtentDims( dims_out, NULL);
    fprintf(stderr, "d1: %lu, d2: %lu \n", (unsigned long)(dims_out[0]), (unsigned long)(dims_out[1]));

    /*
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
    */

    std::vector<std::vector<std::string>> strings(number_of_values);

//    for (auto i = 0; i < number_of_values; ++i) {
//        strings[i] = (*buffer)[i];
//    }

    return strings;
}

auto dataset_to_float_vector(const H5::DataSet &dataSet) -> std::vector<float> {
    const auto number_of_values = dataSet.getSpace().getSimpleExtentNpoints();

    std::vector<float> buffer (number_of_values);

    dataSet.read(static_cast<void*>(buffer.data()), dataSet.getDataType());

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

    // TODO: CSV?
    return {attribute_to_string(attribute)};
}

/// Parses ebv subgroup levels
///
/// Assumes that the attribute is non-empty
auto NetCdfParser::ebv_subgroup_levels() const -> std::vector<std::vector<std::string>> {
    const auto attribute = file.openAttribute("ebv_subgroups_levels");

    return attribute_to_2d_string_vector(attribute);

    /*
    const auto raw_values = attribute_to_string(attribute);

    std::vector<std::vector<std::string>> subgroup_levels = {std::vector<std::string>()};
    auto level = 0;

    std::stringstream subgroup_value;
    for (const auto &c : raw_values) {
        if (c == ';') {
            subgroup_levels[level].push_back(subgroup_value.str());
            subgroup_value.str("");

            // new vector
            subgroup_levels.emplace_back();
            level += 1;
        } else if (c == ',') {
            subgroup_levels[level].push_back(subgroup_value.str());
            subgroup_value.str("");
        } else {
            subgroup_value << c;
        }
    }
    subgroup_levels[level].push_back(subgroup_value.str()); // append last value

    return subgroup_levels;
     */
}

auto NetCdfParser::ebv_entity_levels() const -> std::vector<std::string> {
    const auto attribute = file.openAttribute("ebv_entity_levels");

    const auto attribute_value = attribute_to_string(attribute);

    std::vector<std::string> strings;
    if (attribute_value == "var_entities") {
        const auto dataset = file.openDataSet("var_entities");

        return dataset_to_string_vector(dataset);
    } else {
        // TODO: is this case possible?
        return {};
    }
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
