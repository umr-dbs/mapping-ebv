
#ifndef MAPPING_GFBIO_NETCDF_PARSER_H
#define MAPPING_GFBIO_NETCDF_PARSER_H


#include <H5Cpp.h>
#include <string>
#include <vector>

class NetCdfParser {
    public:
        explicit NetCdfParser(const std::string& path) : file(H5::H5File(path, H5F_ACC_RDONLY)) {}

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

    private:
        H5::H5File file;
};


#endif //MAPPING_GFBIO_NETCDF_PARSER_H
