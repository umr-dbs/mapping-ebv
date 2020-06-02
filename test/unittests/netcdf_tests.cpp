
#include <gtest/gtest.h>
#include <H5Cpp.h>
#include "util.h"

TEST(NetCdfParser, MultiDimStringVariable) {
    auto file = H5::H5File(test_util::get_data_dir() + "test.nc", H5F_ACC_RDONLY);

    auto variable = file.openDataSet("bar");

    ASSERT_EQ(variable.getSpace().getSimpleExtentNpoints(), 4);

    std::vector<char *> chars_raw(variable.getSpace().getSimpleExtentNpoints());
    variable.read(static_cast<void * >(chars_raw.data()), variable.getDataType());

    std::vector<std::string> chars;
    chars.reserve(chars_raw.size());
    for (const auto &c: chars_raw) {
        chars.emplace_back(c);
    }

    ASSERT_EQ(chars, (std::vector<std::string>{"a", "b", "hello", "you"}));

    ASSERT_EQ(variable.getSpace().getSimpleExtentNdims(), 2);

    hsize_t dims[2];
    variable.getSpace().getSimpleExtentDims(dims);

    ASSERT_EQ(dims[0], 2);
    ASSERT_EQ(dims[1], 2);

    auto attribute = file.openAttribute("multistringattr");

    ASSERT_EQ(attribute.getSpace().getSimpleExtentNpoints(), 3);

    std::vector<char *> attribute_strings_raw(attribute.getSpace().getSimpleExtentNpoints());
    attribute.read(attribute.getDataType(), static_cast<void * >(attribute_strings_raw.data()));

    std::vector<std::string> attribute_strings;
    attribute_strings.reserve(attribute_strings_raw.size());
    for (const auto &c: attribute_strings_raw) {
        attribute_strings.emplace_back(c);
    }

    ASSERT_EQ(attribute_strings, (std::vector<std::string>{"apples", "foobar", "cowboy"}));
}
