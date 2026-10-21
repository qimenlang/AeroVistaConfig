#pragma once

#include <nlohmann/json_fwd.hpp>

#include <string>

namespace aerovista::config
{
    /// 通道偏移：叠加在 Host 眼点之上。JSON `offsetDeg` 写 `heading`/`pitch`/`roll`；
    /// `heading` 是顺时针航向，读入时 `yaw = -heading`，之后只使用右手 yaw。
    struct OffsetDeg
    {
        double yaw = 0.0;
        double pitch = 0.0;
        double roll = 0.0;
    };

    /// IG 侧配置。成员名与 `igConfig` JSON 键相同。
    struct IgConfig
    {
        int udpRecvPort = 0;
        std::string hostAddr;
        int hostTcpPort = 0;
        int hostUdpRecvPort = 0;
    };

    /// 从已解析的 JSON 对象解析 `igConfig` 块。
    IgConfig parseIgConfig(const nlohmann::json& obj);

    /// 解析只含 `igConfig` 块的文件。顶层未知键拒绝。
    bool loadIgConfig(const std::string& path, IgConfig& out, std::string* error = nullptr);
} // namespace aerovista::config
