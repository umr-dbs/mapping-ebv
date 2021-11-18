#include "services/httpservice.h"
#include "userdb/userdb.h"
#include "util/concat.h"
#include "util/curl.h"

#include <algorithm>
#include <util/log.h>
#include <util/netcdf_parser.h>
#include <util/stringsplit.h>
#include <boost/algorithm/string.hpp>

/// This class provides methods for user authentication with OpenId Connect
class GeoBonCatalogService : public HTTPService {
    public:
        using HTTPService::HTTPService;

        ~GeoBonCatalogService() override = default;

        struct GeoBonCatalogServiceException : public std::runtime_error {
            using std::runtime_error::runtime_error;
        };

    protected:
        /// Dispatch requests
        void run() override;

        /// Load and return an ID specified EBV dataset from the catalog
        void dataset(UserDB::User &user, const std::string &id) const;

        /// Load and return all EBV classes from the catalog
        void classes() const;

        /// Load and return all EBV datasets from the catalog
        void datasets(UserDB::User &user, const std::string &ebv_name) const;

        /// Extract and return EBV dataset subgroups
        void subgroups(UserDB::User &user, const std::string &ebv_file) const;

        /// Extract and return EBV subgroup values
        void subgroup_values(UserDB::User &user,
                             const std::string &ebv_file,
                             const std::string &ebv_subgroup,
                             const std::vector<std::string> &ebv_group_path) const;

        /// Extract and return meta data for loading the dataset
        void data_loading_info(UserDB::User &user,
                               const std::string &ebv_file,
                               // const std::vector<std::string> &ebv_entity_path) const;
                               const std::string &ebv_entity_path) const;

    private:
        struct EbvClass {
            std::string name;
            std::vector<std::string> ebv_names;

            auto to_json() const -> Json::Value;
        };

        struct Dataset {
            std::string id;
            std::string name;
            std::string author;
            std::string description;
            std::string license;
            std::string dataset_path;

            auto to_json() const -> Json::Value;
        };

        static auto hasUserPermissions(UserDB::User &user, const std::string &ebv_file) -> bool;

        static void addUserPermissions(UserDB::User &user, const std::string &ebv_file);

        static auto requestJsonFromUrl(const std::string &url) -> Json::Value;

        static auto combinePaths(const std::string &first, const std::string &second) -> std::string;

        template<class T>
        static auto toJsonArray(const std::vector<T> &vector) -> Json::Value;

        static auto subgroups_json(const NetCdfParser &net_cdf_parser) -> Json::Value;
};

REGISTER_HTTP_SERVICE(GeoBonCatalogService, "geo_bon_catalog"); // NOLINT(cert-err58-cpp)


void GeoBonCatalogService::run() {
    try {
        const auto session = UserDB::loadSession(params.get("sessiontoken"));

        const std::string &request = params.get("request");

        if (request == "dataset") {
          this->dataset(session->getUser(), params.get("id"));
        } else if (request == "classes") {
            this->classes();
        } else if (request == "datasets") {
            this->datasets(session->getUser(), params.get("ebv_name"));
        } else if (request == "subgroups") {
            this->subgroups(session->getUser(), params.get("ebv_path"));
        } else if (request == "subgroup_values") {
            this->subgroup_values(session->getUser(),
                                  params.get("ebv_path"),
                                  params.get("ebv_subgroup"),
                                  split(params.get("ebv_group_path"), '/'));
        } else if (request == "data_loading_info") {
            this->data_loading_info(session->getUser(),
                                    params.get("ebv_path"),
                                    // split(params.get("ebv_entity_path"), '/'));
                                    params.get("ebv_entity_path"));
        } else { // FALLBACK
            response.sendFailureJSON("GeoBonCatalogService: Invalid request");
        }

    } catch (const std::exception &e) {
        response.sendFailureJSON(e.what());
    }
}

void GeoBonCatalogService::dataset(UserDB::User &user, const std::string &id) const {
    // Dataset info

    const auto web_service_json = requestJsonFromUrl(combinePaths(
            Configuration::get<std::string>("ebv.webservice_endpoint"),
            concat("datasets/", boost::algorithm::replace_all_copy(id, " ", "%20"))
    ));

    const auto dataset = web_service_json.get("data", Json::Value(Json::objectValue))[0];

    const std::string dataset_path = combinePaths(
            Configuration::get<std::string>("ebv.path"),
            dataset.get("dataset", "").get("pathname", "").asString()
    );
    GeoBonCatalogService::addUserPermissions(user, dataset_path);

    // Subgroups

    NetCdfParser net_cdf_parser(dataset_path);
    auto subgroups = subgroups_json(net_cdf_parser);

    // Add values to each subgroup

    std::vector<std::string> ebv_group_path;

    Json::Value subgroups_array (Json::arrayValue);
    for (auto &subgroup : subgroups) {
        const auto ebv_subgroup = subgroup["name"].asString();

        bool first_value_in_subgroup = true;

        Json::Value values(Json::arrayValue);
        for (const auto &subgroup_value : net_cdf_parser.ebv_subgroup_values(ebv_subgroup, ebv_group_path)) {
            if (first_value_in_subgroup) {
                ebv_group_path.push_back(subgroup_value.name);
                first_value_in_subgroup = false;
            }

            values.append(subgroup_value.to_json());
        }

        subgroup["values"] = values;
        subgroups_array.append(subgroup);
    }

    Json::Value result(Json::objectValue);
    result["dataset"] = dataset;
    result["subgroups_and_values"] = subgroups_array;

    response.sendSuccessJSON(result);
}

void GeoBonCatalogService::classes() const {
    const auto web_service_json = requestJsonFromUrl(combinePaths(
            Configuration::get<std::string>("ebv.webservice_endpoint"),
            "ebv"
    ));

    Json::Value datasets(Json::arrayValue);
    for (const auto &dataset : web_service_json["data"]) {
        std::vector<std::string> ebv_names;

	const auto ebv_names_json = dataset.get("ebvName", Json::Value(Json::arrayValue));

        ebv_names.reserve(ebv_names.size());
        for (const auto &ebvName : ebv_names_json) {
            ebv_names.push_back(ebvName.asString());
        }

        datasets.append(GeoBonCatalogService::EbvClass{
                .name = dataset.get("ebv", "").get("ebv_class", "").asString(),
                .ebv_names = ebv_names,
        }.to_json());
    }

    Json::Value result(Json::objectValue);
    result["classes"] = datasets;

    response.sendSuccessJSON(result);
}

void GeoBonCatalogService::datasets(UserDB::User &user, const std::string &ebv_name) const {
    const auto web_service_json = requestJsonFromUrl(combinePaths(
            Configuration::get<std::string>("ebv.webservice_endpoint"),
            concat("datasets/filter?ebvName=", boost::algorithm::replace_all_copy(ebv_name, " ", "+"))
    ));

    Json::Value datasets(Json::arrayValue);
    for (const auto &dataset : web_service_json["data"]) {
        const std::string dataset_path = combinePaths(
                Configuration::get<std::string>("ebv.path"),
                dataset.get("dataset", "").get("pathname", "").asString()
        );

        GeoBonCatalogService::addUserPermissions(user, dataset_path);

        datasets.append(GeoBonCatalogService::Dataset{
                .id = dataset.get("id", "").asString(),
                .name = dataset.get("title", "").asString(),
                .author = dataset.get("creator", "").get("creator_name", "").asString(),
                .description = dataset.get("summary", "").asString(),
                .license = dataset.get("license", "").asString(),
                .dataset_path = dataset_path,
        }.to_json());
    }

    Json::Value result(Json::objectValue);
    result["datasets"] = datasets;

    response.sendSuccessJSON(result);
}

void GeoBonCatalogService::subgroups(UserDB::User &user, const std::string &ebv_file) const {
    if (!hasUserPermissions(user, ebv_file)) {
        throw GeoBonCatalogServiceException(concat("GeoBonCatalogServiceException: Missing access rights for ", ebv_file));
    }

    NetCdfParser net_cdf_parser(ebv_file);

    Json::Value result(Json::objectValue);
    result["subgroups"] = subgroups_json(net_cdf_parser);

    response.sendSuccessJSON(result);
}

auto GeoBonCatalogService::subgroups_json(const NetCdfParser &net_cdf_parser) -> Json::Value {
    const auto subgroup_names = net_cdf_parser.ebv_subgroups();
    const auto subgroup_descriptions = net_cdf_parser.ebv_subgroup_descriptions();

    Json::Value subgroups_json(Json::arrayValue);
    for (size_t i = 0; i < subgroup_names.size(); ++i) {
        Json::Value subgroup_json(Json::objectValue);
        subgroup_json["name"] = subgroup_names[i];
        if (subgroup_descriptions.size() > i) { // TODO: remove, once data format is consistent
            subgroup_json["description"] = subgroup_descriptions[i];
        } else {
            subgroup_json["description"] = "";
        }
        subgroups_json.append(subgroup_json);
    }

    return subgroups_json;
}

void GeoBonCatalogService::subgroup_values(UserDB::User &user,
                                           const std::string &ebv_file,
                                           const std::string &ebv_subgroup,
                                           const std::vector<std::string> &ebv_group_path) const {
    if (!hasUserPermissions(user, ebv_file)) {
        throw GeoBonCatalogServiceException(concat("GeoBonCatalogServiceException: Missing access rights for ", ebv_file));
    }

    NetCdfParser net_cdf_parser(ebv_file);

    Json::Value values(Json::arrayValue);
    for (const auto &subgroup : net_cdf_parser.ebv_subgroup_values(ebv_subgroup, ebv_group_path)) {
        values.append(subgroup.to_json());
    }

    Json::Value result(Json::objectValue);
    result["values"] = values;

    response.sendSuccessJSON(result);
}

void GeoBonCatalogService::data_loading_info(UserDB::User &user,
                                             const std::string &ebv_file,
                                             // const std::vector<std::string> &ebv_entity_path) const {
                                             const std::string &ebv_entity_path) const {
    if (!hasUserPermissions(user, ebv_file)) {
        throw GeoBonCatalogServiceException(concat("GeoBonCatalogServiceException: Missing access rights for ", ebv_file));
    }

    NetCdfParser net_cdf_parser(ebv_file);
    const auto time_info = net_cdf_parser.time_info();
    const auto unit_range = net_cdf_parser.unit_range(ebv_entity_path);

    Json::Value result(Json::objectValue);
    result["time_points"] = toJsonArray(time_info.time_points_unix);
    result["delta_unit"] = time_info.delta_unit;
    result["crs_code"] = net_cdf_parser.crs_as_code();
    result["unit_range"] = toJsonArray(std::vector<double>{unit_range[0], unit_range[1]});

    response.sendSuccessJSON(result);
}

void GeoBonCatalogService::addUserPermissions(UserDB::User &user, const std::string &ebv_file) {
    const std::string permission = concat("data.gdal_source.", ebv_file);

    if (!user.hasPermission(permission)) {
        user.addPermission(permission);
    }
}

auto GeoBonCatalogService::hasUserPermissions(UserDB::User &user, const std::string &ebv_file) -> bool {
    const std::string permission = concat("data.gdal_source.", ebv_file);

    return user.hasPermission(permission);
}

auto GeoBonCatalogService::requestJsonFromUrl(const std::string &url) -> Json::Value {
    cURL curl;
    std::stringstream data;

    curl.setOpt(CURLOPT_PROXY, Configuration::get<std::string>("proxy", "").c_str());
    curl.setOpt(CURLOPT_URL, url.c_str());
    curl.setOpt(CURLOPT_WRITEFUNCTION, cURL::defaultWriteFunction);
    curl.setOpt(CURLOPT_WRITEDATA, &data);
    curl.perform();

    std::string json = data.str();

    Json::Reader reader(Json::Features::strictMode());
    Json::Value result;
    if (!reader.parse(json, result)) {
        throw GeoBonCatalogServiceException(concat(
                "GeoBonCatalogServiceException: Could not parse from JSON response: ", json
        ));
    }

    return result;
}

auto GeoBonCatalogService::combinePaths(const std::string &first, const std::string &second) -> std::string {
    const bool firstEndsWithSlash = first.back() == '/';
    const bool secondStartsWithSlash = second.front() == '/';

    if (firstEndsWithSlash != secondStartsWithSlash) {
        return concat(first, second);
    } else if (firstEndsWithSlash && secondStartsWithSlash) {
        return concat(first.substr(0, first.length() - 1), second);
    } else {
        return concat(first, '/', second);
    }
}

template<class T>
auto GeoBonCatalogService::toJsonArray(const std::vector<T> &vector) -> Json::Value {
    Json::Value array(Json::arrayValue);
    for (const auto &v : vector) {
        array.append(v);
    }
    return array;
}

auto GeoBonCatalogService::Dataset::to_json() const -> Json::Value {
    Json::Value json(Json::objectValue);
    json["id"] = this->id;
    json["name"] = this->name;
    json["author"] = this->author;
    json["description"] = this->description;
    json["license"] = this->license;
    json["dataset_path"] = this->dataset_path;
    return json;
}

auto GeoBonCatalogService::EbvClass::to_json() const -> Json::Value {
    Json::Value ebv_names_json(Json::arrayValue);
    for (const auto &ebv_name : this->ebv_names) {
        ebv_names_json.append(ebv_name);
    }

    Json::Value json(Json::objectValue);
    json["name"] = this->name;
    json["ebv_names"] = ebv_names_json;

    return json;
}
