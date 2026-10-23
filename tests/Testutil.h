#pragma once

// 测试辅助：临时 JSON、通道片段和字段比较。

#include <aerovista/config/EngineChannelConfig.h>
#include <aerovista/config/EntityCatalog.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using aerovista::config::EntityAuthorityPose;
using aerovista::config::EntityAuthorityRow;
using aerovista::config::OffsetDeg;

class TempConfigFile
{
public:
    explicit TempConfigFile(const std::string& jsonBody)
    {
        _path = (std::filesystem::temp_directory_path() /
                 ("ave_engine_cfg_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)) + ".json"))
                    .string();
        std::ofstream out(_path, std::ios::binary);
        out << jsonBody;
    }

    ~TempConfigFile()
    {
        std::error_code ec;
        std::filesystem::remove(_path, ec);
    }

    const std::string& path() const { return _path; }

private:
    std::string _path;
};

inline std::string jsonIgConfig(int udpRecv = 8003)
{
    return std::string(R"("igConfig": { "udpRecvPort": )") + std::to_string(udpRecv) +
           R"(, "hostAddr": "127.0.0.1", "hostTcpPort": 8100, "hostUdpRecvPort": 8000 })";
}

inline bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) <= 1e-9;
}

inline bool offsetEquals(const OffsetDeg& a, const OffsetDeg& b)
{
    return nearlyEqual(a.yaw, b.yaw) && nearlyEqual(a.pitch, b.pitch) && nearlyEqual(a.roll, b.roll);
}

inline const EntityAuthorityRow* findRow(const std::vector<EntityAuthorityRow>& rows, std::uint16_t entityId)
{
    for (const EntityAuthorityRow& row : rows)
    {
        if (row.entityId == entityId)
            return &row;
    }
    return nullptr;
}

inline bool poseIsZero(const EntityAuthorityPose& pose)
{
    return pose.lat == 0.0 && pose.lon == 0.0 && pose.alt == 0.0 && pose.yawDeg == 0.0 && pose.pitchDeg == 0.0 &&
           pose.rollDeg == 0.0;
}
