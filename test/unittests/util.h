
#ifndef MAPPING_CORE_UTIL_H
#define MAPPING_CORE_UTIL_H

#include <string>


class test_util {
    public:
        static auto get_data_dir() -> std::string {
            std::string file(__FILE__);
            const auto path_postfix = "unittests/";
            auto last_slash = file.rfind(path_postfix);
            return file.substr(0, last_slash) + "data/";
        }
};


#endif //MAPPING_CORE_UTIL_H
