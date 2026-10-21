#pragma once

#include <aerovista/config/IgConfig.h>

#include <optional>
#include <string>
#include <vector>

namespace aerovista::config
{
    /// JSON `window`：`posX` / `posY` / `width` / `height`。
    struct WindowConfig
    {
        int posX = 0;
        int posY = 0;
        int width = 1920;
        int height = 1080;
    };

    struct Vec3Config
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    struct LocalPoseConfig
    {
        Vec3Config position{};
        Vec3Config eulerYprDeg{};
    };

    struct EllipsoidPoseConfig
    {
        Vec3Config lla{}; // lat°, lon°, alt m
        Vec3Config eulerYprDeg{};
    };

    /// 实体初始显隐状态（entities.json `initialEntityState`）。
    /// 仅 Active/Standby 两值（缺省 Active）；Destroyed/Remove 及非法值在解析期 fail-fast。
    enum class EntityInitialState
    {
        STANDBY = 0,
        ACTIVE = 1
    };

    /// One `entities[]` item。
    struct EntityConfig
    {
        int id = 0;
        std::string name;
        std::string model;
        EntityInitialState initialEntityState = EntityInitialState::ACTIVE;
        bool hasPose = false;
        bool hasPoseLocal = false;
        bool hasPoseEllipsoid = false;
        LocalPoseConfig localPose{};
        EllipsoidPoseConfig ellipsoidPose{};
    };

    struct CameraConfig
    {
        bool hasPose = false;
        bool hasPoseLocal = false;
        bool hasPoseEllipsoid = false;
        LocalPoseConfig localPose{};
        EllipsoidPoseConfig ellipsoidPose{};
    };

    /// 接缝目视用 ENU 切平面线网格。钉到 `lla` 的 LocalToWorld，不是瓦片地球。
    struct GroundGridConfig
    {
        Vec3Config lla{}; // lat°, lon°, alt m。切平面原点。
        // 地面半边长（米）。整块地面边长 = 2 * halfGroundLenM。缺省 20000，整幅约 40 km。
        double halfGroundLenM = 20000.0;
        // 单格边长（米），不是整块地面的边长。缺省 200。
        // 格数 = round(2 * halfGroundLenM / gridLenM)，须在 1..400。
        double gridLenM = 200.0;
        // 每隔多少条格线画一条主线（白），其余为辅线（黄）。缺省 5。须 >= 1。
        int majorLineEvery = 5;
    };

    /// JSON `viewDefinition`。`viewID` 写入 IgSync HELLO 的 channelId 字段；
    /// `requireConnectedIg` 由 Engine::initSync 消费；`offsetDeg` 由 CameraDriver 消费。
    /// `near` / `far` 的成员叫 nearClip / farClip：Windows 历史宏会吞掉 near、far 这两个名字。
    struct ViewDefinition
    {
        int viewID = 0;
        OffsetDeg offsetDeg{};
        bool requireConnectedIg = false;
        /// 垂直视场角（度）。JSON `vfov`，缺省 30。水平视场由窗口宽高比推出。
        double vfov = 30.0;
        /// 近远比的两端。JSON `near` / `far`，缺省 1 与 1000，比值 0.001。
        /// 不是裁剪面的绝对距离：本地 near = 比值 × radius，椭球 near = 动态 far × 比值。
        double nearClip = 1.0;
        double farClip = 1000.0;
        WindowConfig window{};

        double nearFarRatio() const
        {
            return nearClip / farClip;
        }
    };

    /// 每进程 Engine 通道配置（见 engine/resources/config/*.json）。
    struct EngineChannelConfig
    {
        ViewDefinition viewDefinition{};

        /// IG 传输配置；nullopt = 未启用同步（无 igConfig）。
        std::optional<IgConfig> igConfig{};

        /// 相机初始位姿；nullopt = 无 camera 键（默认相机走场景 AABB）。
        std::optional<CameraConfig> camera{};

        std::string model = "models/lz.vsgt";
        /// 场景模型无自带 EllipsoidModel 时，是否注入一个 WGS-84 椭球。
        /// 仅对「单机椭球渲染」（无 igConfig）生效；启用 IG 同步时引擎自动注入。
        bool injectEllipsoidIfMissing = false;

        /// 实体目录文件路径（entities.json）；空 = 不装实体（单模型）。
        std::string entitiesFilePath;

        /// 可选接缝网格；出现则 `lla` 必填。需要场景 EllipsoidModel（注入或自带）。
        std::optional<GroundGridConfig> groundGrid{};
    };

    bool loadEngineChannelConfig(const std::string& path, EngineChannelConfig& out, std::string* error = nullptr);

    /// 从独立实体目录文件 `entities.json` 解析 EntityConfig 列表（顶层 `{ "entities": [...] }`）。
    bool loadEntitiesFile(const std::string& path, std::vector<EntityConfig>& out, std::string* error = nullptr);
} // namespace aerovista::config
