#include "services/httpservice.h"
#include "userdb/userdb.h"
#include "util/concat.h"
#include "util/curl.h"

#include <algorithm>
#include <util/log.h>
#include <util/netcdf_parser.h>
#include <util/stringsplit.h>

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

        /// Extract and return the EBV time dimension
        void data_loading_info
    (UserDB::User &user,
                            const std::string &ebv_file) const;

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
            std::string wkt_code;

            auto to_json() const -> Json::Value;
        };

        static auto hasUserPermissions(UserDB::User &user, const std::string &ebv_file) -> bool;

        static void addUserPermissions(UserDB::User &user, const std::string &ebv_file);

        static auto requestJsonFromUrl(const std::string &url) -> Json::Value;

        static auto combinePaths(const std::string &first, const std::string &second) -> std::string;

        template<class T>
        static auto toJsonArray(const std::vector<T> &vector) -> Json::Value;
};

REGISTER_HTTP_SERVICE(GeoBonCatalogService, "geo_bon_catalog"); // NOLINT(cert-err58-cpp)


void GeoBonCatalogService::run() {
    try {
        const auto session = UserDB::loadSession(params.get("sessiontoken"));

        const std::string &request = params.get("request");

        if (request == "classes") {
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
            this->data_loading_info
        (session->getUser(), params.get("ebv_path"));
        } else { // FALLBACK
            response.sendFailureJSON("GeoBonCatalogService: Invalid request");
        }

    } catch (const std::exception &e) {
        response.sendFailureJSON(e.what());
    }
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
                .name = dataset.get("ebvClass", "").asString(),
                .ebv_names = ebv_names,
        }.to_json());
    }

    Json::Value result(Json::objectValue);
    result["classes"] = datasets;

    response.sendSuccessJSON(result);
}

void GeoBonCatalogService::datasets(UserDB::User &user, const std::string &ebv_name) const {
    //    const auto web_service_json = requestJsonFromUrl(combinePaths(
    //            Configuration::get<std::string>("ebv.webservice_endpoint"),
    //            concat("datasets/ebvName/", boost::algorithm::replace_all_copy(ebv_name, " ", "%20"))
    //    ));
    // TODO: use new endpoint when ready
    const auto web_service_json = requestJsonFromUrl(combinePaths(
            Configuration::get<std::string>("ebv.webservice_endpoint"),
            "datasets/list"
    ));

    // TODO: remove after testing
    int remove_i = 0;

    Json::Value datasets(Json::arrayValue);
    for (const auto &dataset : web_service_json["data"]) {
        // TODO: remove filter and exchange with correct service endpoint
//        const std::string dataset_ebv_name = dataset
//                .get("ebv", Json::Value(Json::objectValue))
//                .get("ebvName", "").asString();
//        if (dataset_ebv_name != ebv_name) continue;

        // TODO: use after testing
//        const std::string dataset_path = combinePaths(
//                Configuration::get<std::string>("ebv.path"),
//                dataset.get("pathNameDataset", "").asString()
//        );

        // TODO: remove after testing
        std::string dataset_path;
        switch (remove_i) {
            case 0:
                dataset_path = combinePaths(
                        Configuration::get<std::string>("ebv.path"),
                        "cSAR_idiv_004.nc"
                );
                break;
            case 1:
                dataset_path = combinePaths(
                        Configuration::get<std::string>("ebv.path"),
                        "hansen_lossyear_1000m.nc"
                );
                break;
            case 2:
                dataset_path = combinePaths(
                        Configuration::get<std::string>("ebv.path"),
                        "hennekens_003.nc"
                );
                break;
            case 3:
                dataset_path = combinePaths(
                        Configuration::get<std::string>("ebv.path"),
                        "RMF_001.nc"
                );
                break;
            default:
                dataset_path = combinePaths(
                        Configuration::get<std::string>("ebv.path"),
                        dataset.get("pathNameDataset", "").asString()
                );
        }
        remove_i += 1;

        GeoBonCatalogService::addUserPermissions(user, dataset_path);

        datasets.append(GeoBonCatalogService::Dataset{
                .id = dataset.get("id", "").asString(),
                .name = dataset.get("name", "").asString(),
                .author = dataset.get("author", "").asString(),
                .description = dataset.get("abstract", "").asString(),
                .license = dataset.get("License", "").asString(),
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

    const auto subgroup_names = net_cdf_parser.ebv_subgroups();
    const auto subgroup_descriptions = net_cdf_parser.ebv_subgroup_descriptions();

    Json::Value subgroups_json(Json::arrayValue);
    for (size_t i = 0; i < subgroup_names.size(); ++i) {
        Json::Value subgroup_json(Json::objectValue);
        subgroup_json["name"] = subgroup_names[i];
        subgroup_json["description"] = subgroup_descriptions[i];
        subgroups_json.append(subgroup_json);
    }

    Json::Value result(Json::objectValue);
    result["subgroups"] = subgroups_json;

    response.sendSuccessJSON(result);
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

void GeoBonCatalogService::data_loading_info(UserDB::User &user, const std::string &ebv_file) const {
    if (!hasUserPermissions(user, ebv_file)) {
        throw GeoBonCatalogServiceException(concat("GeoBonCatalogServiceException: Missing access rights for ", ebv_file));
    }

    NetCdfParser net_cdf_parser(ebv_file);
    const auto time_info = net_cdf_parser.time_info();

    Json::Value result(Json::objectValue);
    result["time_points"] = toJsonArray(time_info.time_points_unix);
    result["delta_unit"] = time_info.delta_unit;

    const auto crs_code = net_cdf_parser.crs_as_code();
    result["crs_code"] = crs_code;

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
