#include "services/httpservice.h"
#include "userdb/userdb.h"
#include "util/concat.h"
#include "util/curl.h"

#include <algorithm>
#include <util/log.h>
#include <util/netcdf_parser.h>

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
        void datasets(const std::string &ebv_name) const;

        /// Extract and return EBV dataset levels
        void levels(const std::string &ebv_file) const;

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
};

REGISTER_HTTP_SERVICE(GeoBonCatalogService, "geo_bon_catalog"); // NOLINT(cert-err58-cpp)


void GeoBonCatalogService::run() {
    try {
        const std::string &request = params.get("request");

        if (request == "classes") {
            this->classes();
        } else if (request == "datasets") {
            this->datasets(params.get("ebv_name", ""));
        } else if (request == "levels") {
            this->levels(params.get("ebv_path", ""));
        } else { // FALLBACK
            response.sendFailureJSON("GeoBonCatalogService: Invalid request");
        }

    } catch (const std::exception &e) {
        response.sendFailureJSON(e.what());
    }
}

void GeoBonCatalogService::classes() const {
    // TODO: call `/ebv-catalog/api/v1/???`

    std::string web_service_string = R"RAWSTRING(
    {
        "code": 200,
        "message": "List of EBV classes and names",
        "data": [
        {
            "ebvClass": "Genetic composition",
            "ebvName": [
                "Co-ancestry",
                "Allelic diversity",
                "Population genetic differentiation",
                "Breed and variety diversity"
            ]
        },
        {
            "ebvClass": "Species populations",
            "ebvName": [
                "Species distribution",
                "Population abundance",
                "Population structure by age/size class"
            ]
        },
        {
            "ebvClass": "Species traits",
            "ebvName": [
                "Phenology",
                "Morphology",
                "Reproduction",
                "Physiology",
                "Movement"
            ]
        },
        {
            "ebvClass": "Community composition",
            "ebvName": [
                "Taxonomic diversity",
                "Species interactions"
            ]
        },
        {
            "ebvClass": "Ecosystem function",
            "ebvName": [
                "Net primary productivity",
                "Secondary productivity",
                "Nutrient retention",
                "Disturbance regime"
            ]
        },
        {
            "ebvClass": "Ecosystem structure",
            "ebvName": [
                "Habitat structure",
                "Ecosystem extent and fragmentation",
                "Ecosystem composition by functional type"
            ]
        }]
    }
    )RAWSTRING";

    Json::Reader reader(Json::Features::strictMode());
    Json::Value web_service_json;

    if (!reader.parse(web_service_string, web_service_json)) {
        Log::error(concat(
                "GeoBonCatalogServiceException: Invalid web service result",
                '\n',
                web_service_string
        ));
        throw GeoBonCatalogService::GeoBonCatalogServiceException("GeoBonCatalogServiceException: Invalid web service result");
    }

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

void GeoBonCatalogService::datasets(const std::string &ebv_name) const {
    // TODO: call `/ebv-catalog/api/v1/datasets/list`
    // TODO: incorporate `ebv_name`

    std::string web_service_string = R"RAWSTRING(
    {
        "code": 200,
        "message": "List of all datasets",
        "data": [
        {
            "id": "1",
            "name": "Changes in local terrestrial diversity (PREDICTS)",
            "author": "PREDICTS",
            "publicationDate": "2020",
            "technicalStatus": "Finish and with constant technical support",
            "ebv":
            {
                "ebvClass": "Community composition",
                "ebvName": "Population genetic differentiation"
            },
            "spatialCoverage": "Global",
            "temporalResolution": "Yearly",
            "taxonomicCoverage": null,
            "License": "CC BY",
            "pathNameDataset": "../../../dataset-uploads/1/cSAR-idiv_sb.nc",
            "pathNameMetadata": "../../../dataset-uploads/1/metadata.xml",
            "abstract": "Changes in average local terrestrial diversity for each grid cell caused by land-use, land-use intensity, and human population density, estimated by the PREDICTS model (Purvis et al., 2018). It reports number of species in each cell relative to a pristine baseline (percentage) and changes in species number (percentage) relative to 1900. Uses the LUH 2.0 projections for land-use and the PREDICTS database with 767 studies from over 32 000 sites on over 51 000 species from all taxa.",
            "contactDetails":
            {
                "contactPerson": "Andy Purvis",
                "contactInstitute": "PREDICTS",
                "contactEmail": "andy.purvis@nhm.ac.uk"
            },
            "keywords": "machine learning,remote sensing"
        },
        {
            "id": "3",
            "name": "Forest cover",
            "author": "Matthew Hansen",
            "publicationDate": "2018",
            "technicalStatus": "Finish and with constant technical support",
            "ebv":
            {
                "ebvClass": "Ecosystem structure",
                "ebvName": "Population genetic differentiation"
            },
            "spatialCoverage": "Global",
            "temporalResolution": "Yearly",
            "taxonomicCoverage": "Plants",
            "License": "Creative Commons Attribution 4.0 International License",
            "pathNameDataset": "../../../dataset-uploads/3/cSAR-idiv_sb.nc",
            "pathNameMetadata": "../../../dataset-uploads/3/metadata.xml",
            "abstract": "Data in this layer were generated using multispectral satellite imagery from the Landsat 7 thematic mapper plus (ETM+) sensor. The clear surface observations from over 600,000 images were analyzed using Google Earth Engine, a cloud platform for earth observation and data analysis, to determine per pixel tree cover using a supervised learning algorithm.",
            "contactDetails":
            {
                "contactPerson": "Matthew Hansen",
                "contactInstitute": "University of Maryland, Department of Geographical Sciences",
                "contactEmail": "mhansen@umd.edu"
            },
            "keywords": "Big data"
        },
        {
            "id": "9",
            "name": "Test EBV christian",
            "author": "University of Amsterdam",
            "publicationDate": "2020",
            "technicalStatus": "In development",
            "ebv":
            {
                "ebvClass": "Ecosystem structure",
                "ebvName": "Population genetic differentiation"
            },
            "spatialCoverage": "Global",
            "temporalResolution": "Weekly",
            "taxonomicCoverage": null,
            "License": "CC BY-ND",
            "pathNameDataset": "../../../dataset-uploads/9/cSAR-idiv_sb.nc",
            "pathNameMetadata": "../../../dataset-uploads/9/metadata.xml",
            "abstract": "test description.\r\n\r\nCras justo odio, dapibus ac facilisis in, egestas eget quam. Nulla vitae elit libero, a pharetra augue. Maecenas faucibus mollis interdum. Donec sed odio dui. Aenean lacinia bibendum nulla sed consectetur. Morbi leo risus, porta ac consectetur ac, vestibulum at eros. Nullam quis risus eget urna mollis ornare vel eu leo.",
            "contactDetails":
            {
                "contactPerson": "Andy Purvis",
                "contactInstitute": "German Centre for Integrative Biodiversity Research (iDiv)",
                "contactEmail": "andy.purvis@nhm.ac.uk"
            },
            "keywords": "machine learning,test example,forest,birds"
        },
        {
            "id": "31",
            "name": "ereererererr",
            "author": "Aaike De Wever",
            "publicationDate": "2017",
            "technicalStatus": "Finish without technical support",
            "ebv":
            {
                "ebvClass": "Genetic composition",
                "ebvName": "Population genetic differentiation"
            },
            "spatialCoverage": "National",
            "temporalResolution": "Yearly",
            "taxonomicCoverage": "Amphibians",
            "License": "CC BY-SA",
            "pathNameDataset": "../../../dataset-uploads/31/cSAR-idiv_sb.nc",
            "pathNameMetadata": "../../../dataset-uploads/31/metadata.xml",
            "abstract": "hgghghghghghghghghghghghhg",
            "contactDetails":
            {
                "contactPerson": "Aaike De Wever",
                "contactInstitute": "Aarhus University",
                "contactEmail": "christian.langer@idiv.de"
            },
            "keywords": "Big data,remote sensing"
        }]
    }
    )RAWSTRING";

    Json::Reader reader(Json::Features::strictMode());
    Json::Value web_service_json;

    if (!reader.parse(web_service_string, web_service_json)) {
        Log::error(concat(
                "GeoBonCatalogServiceException: Invalid web service result",
                '\n',
                web_service_string
        ));
        throw GeoBonCatalogService::GeoBonCatalogServiceException("GeoBonCatalogServiceException: Invalid web service result");
    }

    Json::Value datasets(Json::arrayValue);
    for (const auto &dataset : web_service_json["data"]) {
        datasets.append(GeoBonCatalogService::Dataset{
                .id = dataset.get("id", "").asString(),
                .name = dataset.get("name", "").asString(),
                .author = dataset.get("author", "").asString(),
                .description = dataset.get("abstract", "").asString(),
                .license = dataset.get("License", "").asString(),
                .dataset_path = dataset.get("pathNameDataset", "").asString(),
        }.to_json());
    }

    Json::Value result(Json::objectValue);
    result["datasets"] = datasets;

    response.sendSuccessJSON(result);
}

void GeoBonCatalogService::levels(const std::string &ebv_file) const {
    // TODO: incorporate EBV path

    NetCdfParser net_cdf_parser(ebv_file);

    Json::Value levels(Json::arrayValue);
    for (const auto &subgroup : net_cdf_parser.ebv_subgroups()) {
        levels.append(subgroup);
    }

    response.sendSuccessJSON(levels);
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
