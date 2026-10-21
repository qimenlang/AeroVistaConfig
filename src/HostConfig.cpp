#include <aerovista/config/ConfigJson.h>
#include <aerovista/config/HostConfig.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace aerovista::config
{
    namespace
    {
        /// `enable=true` 才填虚 IG；关闭时 JSON 里的 igConfig / expectedIgCount 忽略。
        void applyRelayBlock(const nlohmann::json& root, HostConfig& out)
        {
            const nlohmann::json* relayValue = find(root, "relay");
            if (!relayValue)
                return;

            const nlohmann::json& relay = requireObject(*relayValue, "relay");
            rejectUnknownKeys(relay, {"enable", "expectedIgCount"});
            if (find(relay, "enable"))
                out.relay.enable = requireBool(relay, "enable");
            if (!out.relay.enable)
                return;

            out.relay.expectedIgCount = requireInt(relay, "expectedIgCount");
            if (out.relay.expectedIgCount < 1)
                throw std::runtime_error("relay.expectedIgCount must be an integer >= 1");
            if (!find(root, "igConfig"))
                throw std::runtime_error("relay.enable requires igConfig");
            out.igConfig = parseIgConfig(requireObjectValue(root, "igConfig"));
            if (out.igConfig->udpRecvPort == out.udpPortRecv)
                throw std::runtime_error("igConfig.udpRecvPort must differ from hostConfig.udpPortRecv");
        }
    } // namespace

    HostConfig parseHostConfig(const nlohmann::json& obj)
    {
        rejectUnknownKeys(obj, {"udpPortRecv", "tcpPort"});
        HostConfig cfg;
        cfg.udpPortRecv = requireInt(obj, "udpPortRecv");
        cfg.tcpPort = requireInt(obj, "tcpPort");
        return cfg;
    }

    bool loadHostConfig(const std::string& path, HostConfig& out, std::string* error)
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
            rejectUnknownKeys(root, {"hostConfig", "relay", "igConfig"});
            out = parseHostConfig(requireObjectValue(root, "hostConfig"));
            applyRelayBlock(root, out);
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
