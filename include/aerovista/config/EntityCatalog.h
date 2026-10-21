#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace aerovista::config
{
    /// Host 权威表实体运行态（对应 CIGI EntityState Active/Standby；不含 Destroyed）。
    enum class EntityAuthorityState
    {
        STANDBY,
        ACTIVE
    };

    /// 表内 last pose（Detach+LLA）。
    struct EntityAuthorityPose
    {
        double lat = 0.0;
        double lon = 0.0;
        double alt = 0.0;
        double yawDeg = 0.0;
        double pitchDeg = 0.0;
        double rollDeg = 0.0;
    };

    /// Host 实体目录一行。动画不进本行。
    struct EntityAuthorityRow
    {
        std::uint16_t entityId = 0;
        std::string name;
        std::string model;
        EntityAuthorityState entityState = EntityAuthorityState::ACTIVE;
        std::uint8_t alpha = 255;
        EntityAuthorityPose pose;
    };

    /// Host 读 entities.json 子集（id / name / model / initialEntityState / pose.ellipsoid）。
    /// 与 `loadEntitiesFile` 分开：不解析 pose.local 双轨，不产出 `EntityConfig`。
    /// 失败时不改 `out`。
    bool loadHostEntityCatalog(const std::string& path,
                               std::vector<EntityAuthorityRow>& out,
                               std::string* error = nullptr);
} // namespace aerovista::config
