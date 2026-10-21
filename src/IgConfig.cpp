#include <aerovista/config/ConfigJson.h>
#include <aerovista/config/IgConfig.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace aerovista::config
{
    IgConfig parseIgConfig(const nlohmann::json& obj)
    {
        rejectUnknownKeys(obj, {"udpRecvPort", "hostAddr", "hostTcpPort", "hostUdpRecvPort"});
        IgConfig cfg;
        cfg.udpRecvPort = requireInt(obj, "udpRecvPort");
        cfg.hostAddr = requireString(obj, "hostAddr");
        cfg.hostTcpPort = requireInt(obj, "hostTcpPort");
        cfg.hostUdpRecvPort = requireInt(obj, "hostUdpRecvPort");
        return cfg;
    }

    bool loadIgConfig(const std::string& path, IgConfig& out, std::string* error)
    {
        try
        {
            std::ifstream in(path);
            if (!in)
            {
                if (error)
                    *error = "failed to open config: " + path;
                return false;
            }

            std::ostringstream oss;
            oss << in.rdbuf();
            const nlohmann::json root = parseJsonText(oss.str());
            if (!root.is_object())
                throw std::runtime_error("root must be a JSON object");
            rejectUnknownKeys(root, {"igConfig"});
            out = parseIgConfig(requireObjectValue(root, "igConfig"));
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
