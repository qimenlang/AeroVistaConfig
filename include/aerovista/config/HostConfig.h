#pragma once

#include <aerovista/config/IgConfig.h>

#include <nlohmann/json_fwd.hpp>

#include <optional>

namespace aerovista::config
{
    /// viewhost 中继开关。仅 `enable=true` 时消费 `HostConfig::igConfig`。
    struct Relay
    {
        bool enable = false;
        int expectedIgCount = 0;
    };

    /// Host 侧本地配置。成员与 `hostConfig` / `relay` JSON 键对应。
    struct HostConfig
    {
        int udpPortRecv = 0;
        int tcpPort = 0;
        Relay relay{};
        /// 虚 IG；`relay.enable=false` 时为空（JSON 有也忽略）。
        std::optional<IgConfig> igConfig;
    };

    /// 从已解析的 JSON 对象解析 `hostConfig` 块（不含 `relay`）。
    HostConfig parseHostConfig(const nlohmann::json& obj);

    /// 解析 Host 进程配置。本地调试只含 `hostConfig`；
    /// 中继时可含 `relay` / `igConfig`。顶层未知键拒绝。
    bool loadHostConfig(const std::string& path, HostConfig& out, std::string* error = nullptr);
} // namespace aerovista::config
