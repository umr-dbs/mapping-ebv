#include <memory>
#include <sstream>
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

    std::vector<std::string> strings (number_of_values);

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

    std::vector<std::string> strings (number_of_values);

    for (auto i = 0; i < number_of_values; ++i) {
        strings[i] = (*buffer)[i];
    }

    return strings;
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

    const auto raw_values = attribute_to_string(attribute);

    std::vector<std::vector<std::string>> subgroup_levels = { std::vector<std::string>() };
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
