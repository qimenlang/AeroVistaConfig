#include <aerovista/config/ConfigJson.h>
#include <aerovista/config/EntityCatalog.h>

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace aerovista::config
{
    namespace
    {
        using Json = nlohmann::json;

        std::string basenameOfModel(const std::string& modelPath)
        {
            const auto slash = modelPath.find_last_of("/\\");
            return slash == std::string::npos ? modelPath : modelPath.substr(slash + 1);
        }

        std::array<double, 3> requireNumberTriple(const Json& obj, const char* key)
        {
            const Json& value = requireValue(obj, key);
            if (!value.is_array())
                throw std::runtime_error(std::string("missing/invalid array: ") + key);
            if (value.size() != 3)
                throw std::runtime_error(std::string("array length must be 3: ") + key);

            std::array<double, 3> out{};
            for (std::size_t i = 0; i < 3; ++i)
            {
                if (!value[i].is_number())
                    throw std::runtime_error(std::string("array elements must be numbers: ") + key);
                out[i] = value[i].get<double>();
            }
            return out;
        }

        EntityAuthorityPose parseEllipsoidPose(const Json& obj)
        {
            rejectUnknownKeys(obj, {"lla", "eulerYprDeg"});
            const Json& lla = requireObjectValue(obj, "lla");
            rejectUnknownKeys(lla, {"lat", "lon", "alt"});

            EntityAuthorityPose pose;
            pose.lat = requireNumber(lla, "lat");
            pose.lon = requireNumber(lla, "lon");
            pose.alt = requireNumber(lla, "alt");
            const std::array<double, 3> ypr = requireNumberTriple(obj, "eulerYprDeg");
            pose.yawDeg = ypr[0];
            pose.pitchDeg = ypr[1];
            pose.rollDeg = ypr[2];
            return pose;
        }

        void applyCatalogPose(const Json& entityObj, EntityAuthorityRow& row)
        {
            const Json* poseValue = find(entityObj, "pose");
            if (!poseValue)
                return;

            const Json& poseObj = requireObject(*poseValue, "pose");
            rejectUnknownKeys(poseObj, {"local", "ellipsoid"});
            if (const Json* local = find(poseObj, "local"))
                requireObject(*local, "local");
            if (const Json* ellipsoid = find(poseObj, "ellipsoid"))
                row.pose = parseEllipsoidPose(requireObject(*ellipsoid, "ellipsoid"));
        }

        EntityAuthorityState parseInitialState(const Json& obj)
        {
            const Json* value = find(obj, "initialEntityState");
            if (!value)
                return EntityAuthorityState::ACTIVE;
            rejectNull(*value, "initialEntityState");
            if (!value->is_string())
                throw std::runtime_error("missing/invalid string: initialEntityState");

            const std::string state = value->get<std::string>();
            if (state == "Active")
                return EntityAuthorityState::ACTIVE;
            if (state == "Standby")
                return EntityAuthorityState::STANDBY;
            throw std::runtime_error("invalid initialEntityState (only \"Active\" or \"Standby\"): " + state);
        }

        std::string parseOptionalName(const Json& obj, const std::string& fallback)
        {
            const Json* value = find(obj, "name");
            if (!value)
                return fallback;
            rejectNull(*value, "name");
            if (!value->is_string())
                throw std::runtime_error("missing/invalid string: name");
            return value->get<std::string>();
        }

        EntityAuthorityRow parseEntityRow(const Json& obj)
        {
            rejectUnknownKeys(obj, {"id", "name", "model", "initialEntityState", "pose"});

            const int id = requireInt(obj, "id");
            if (id < 1 || id > 65535)
                throw std::runtime_error("entity id out of range 1..65535");

            EntityAuthorityRow row;
            row.entityId = static_cast<std::uint16_t>(id);
            row.model = requireString(obj, "model");
            if (row.model.empty())
                throw std::runtime_error("entities[].model must be non-empty");
            row.name = parseOptionalName(obj, basenameOfModel(row.model));
            row.entityState = parseInitialState(obj);
            row.alpha = 255;
            applyCatalogPose(obj, row);
            return row;
        }

        std::vector<EntityAuthorityRow> parseEntitiesArray(const Json& value)
        {
            rejectNull(value, "entities");
            if (!value.is_array())
                throw std::runtime_error("entities must be an array");
            if (value.empty())
                throw std::runtime_error("entities must not be empty");

            std::vector<EntityAuthorityRow> rows;
            rows.reserve(value.size());
            std::unordered_set<int> seenIds;
            for (const Json& item : value)
            {
                const EntityAuthorityRow row = parseEntityRow(requireObject(item, "entities[]"));
                if (!seenIds.insert(row.entityId).second)
                    throw std::runtime_error("duplicate entity id");
                rows.push_back(row);
            }
            return rows;
        }
    } // namespace

    bool loadHostEntityCatalog(const std::string& path, std::vector<EntityAuthorityRow>& out, std::string* error)
    {
        try
        {
            std::ifstream in(path);
            if (!in)
            {
                if (error)
                    *error = "failed to open entities file: " + path;
                return false;
            }

            std::ostringstream oss;
            oss << in.rdbuf();
            const Json rootValue = parseJsonText(oss.str());
            if (!rootValue.is_object())
                throw std::runtime_error("entities file root must be a JSON object");

            rejectUnknownKeys(rootValue, {"entities"});
            const Json* entitiesValue = find(rootValue, "entities");
            if (!entitiesValue)
                throw std::runtime_error("missing key: entities");
            out = parseEntitiesArray(*entitiesValue);
            return true;
        }
        catch (const std::exception& ex)
        {
            if (error)
                *error = ex.what();
            return false;
        }
    }
} // namespace aerovista::config
