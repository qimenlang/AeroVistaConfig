#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <aerovista/config/EngineChannelConfig.h>

#include "Testutil.h"

#include <string>

using aerovista::config::EngineChannelConfig;
using aerovista::config::OffsetDeg;
using aerovista::config::loadEngineChannelConfig;

namespace
{
    const char* kDefaultJson = R"({"model":"models/lz.vsgt","viewDefinition":{"vfov":30.0,"near":1.0,"far":1000.0,"window":{"posX":0,"posY":0,"width":1920,"height":1080}}})";
    const char* kMainJson =
        R"({"viewDefinition":{"viewID":0,"offsetDeg":{"heading":0.0,"pitch":0.0,"roll":0.0},"requireConnectedIg":true,"vfov":30.0,"near":1.0,"far":1000.0,"window":{"posX":640,"posY":0,"width":640,"height":1080}},"injectEllipsoidIfMissing":true,"igConfig":{"udpRecvPort":8001,"hostAddr":"127.0.0.1","hostTcpPort":8100,"hostUdpRecvPort":8000},"model":"models/lz.vsgt"})";
    const char* kLeftJson =
        R"({"viewDefinition":{"viewID":1,"offsetDeg":{"heading":-18.05,"pitch":0.0,"roll":0.0},"requireConnectedIg":false,"vfov":30.0,"near":1.0,"far":1000.0,"window":{"posX":0,"posY":0,"width":640,"height":1080}},"injectEllipsoidIfMissing":true,"igConfig":{"udpRecvPort":8003,"hostAddr":"127.0.0.1","hostTcpPort":8100,"hostUdpRecvPort":8000},"model":"models/lz.vsgt"})";
    const char* kMinimalWindow =
        R"("viewDefinition": { "vfov": 30.0, "near": 1.0, "far": 1000.0, "window": { "posX": 0, "posY": 0, "width": 640, "height": 480 } })";
    const char* kMinimalModel = R"("model": "models/lz.vsgt")";

} // namespace

// =============================================================================
// 通道文件出现 schema 未声明的顶层键时整文件拒绝
// （多通道同步模块设计.md §3.1，码 CFG-reject-unknown-top）。
// hostLocl 是笔误键；shaderCube 已从 schema 删除，同样走未知键，不单列码。
// 只验失败且错误非空，不钉错误文本。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects unknown top-level keys", "[unit][config][parse][CFG-reject-unknown-top]")
{
    EngineChannelConfig cfg;
    std::string error;
    const TempConfigFile typo(std::string("{") + kMinimalModel + ", " + kMinimalWindow +
                              R"(, "hostLocl": { "addr": "127.0.0.1" }})");
    REQUIRE_FALSE(loadEngineChannelConfig(typo.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());

    error.clear();
    const TempConfigFile shaderCube(std::string("{") + kMinimalModel + ", " + kMinimalWindow +
                                    R"(, "shaderCube": { "fragment": "shaders/debug_cube.frag" }})");
    REQUIRE_FALSE(loadEngineChannelConfig(shaderCube.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// window 里多写 fullscreen 也拒绝（多通道同步模块设计.md §3.1，码 CFG-reject-unknown-nested）。
// 未知键规则套在嵌套对象上，不只根。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects unknown nested keys", "[unit][config][parse][CFG-reject-unknown-nested]")
{
    const TempConfigFile file(std::string("{") + kMinimalModel +
                              R"(, "viewDefinition": { "window": { "posX": 0, "posY": 0, "width": 640, "height": 480, "fullscreen": true } }})");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 有完整 igConfig、不写 requireConnectedIg 时可以加载
// （多通道同步模块设计.md §3.1，码 CFG-accept-igconfig-no-require）。
// igConfig 自带本端端口和 Host 地址，不再另配 hostEndpoint。不验该布尔的缺省值。
// =============================================================================

TEST_CASE("loadEngineChannelConfig accepts igConfig without requireConnectedIg", "[unit][config][parse][CFG-accept-igconfig-no-require]")
{
    const TempConfigFile file(std::string(R"({ "injectEllipsoidIfMissing": true, )") + jsonIgConfig() + ", " + kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.igConfig.has_value());
}

// =============================================================================
// requireConnectedIg 为 true 但没有 igConfig 则拒绝
// （多通道同步模块设计.md §3.1，码 CFG-reject-require-without-ig）。
// 要连上 Host 就必须有地址块。false 和缺省不在本例。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects requireConnectedIg without igConfig", "[unit][config][parse][CFG-reject-require-without-ig]")
{
    const TempConfigFile file(
        std::string(R"({ "viewDefinition": { "requireConnectedIg": true, "window": { "posX": 0, "posY": 0, "width": 640, "height": 480 } }, )") +
        kMinimalModel + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// window 只写 width、缺其余字段则拒绝（多通道同步模块设计.md §3.1 方案 A，码 CFG-reject-partial-window）。
// 对象一旦出现，posX / posY / width / height 全必填。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects partial window object (scheme A)", "[unit][config][parse][CFG-reject-partial-window]")
{
    const TempConfigFile file(std::string("{") + kMinimalModel +
                              R"(, "viewDefinition": { "window": { "width": 800 } }})");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}


// =============================================================================
// offsetDeg 只写 heading、缺 pitch/roll 时拒绝加载
// （多通道同步模块设计.md §3.1 方案 A，码 CFG-reject-partial-offset）。
// 键名已是 heading；本例只验半填对象被拒，不验取反后的数值。
// =============================================================================
TEST_CASE("loadEngineChannelConfig rejects partial offsetDeg object (scheme A)", "[unit][config][parse][CFG-reject-partial-offset]")
{
    const TempConfigFile file(std::string("{") + kMinimalModel +
                              R"(, "viewDefinition": { "offsetDeg": { "heading": 1.0 }, "window": { "posX": 0, "posY": 0, "width": 640, "height": 480 } }})");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 通道文件里的 hostConfig 是未知顶层键，整文件拒绝
// （多通道同步模块设计.md §3.1，码 CFG-reject-hostconfig-top）。
// Host 端口只属于 loadHostConfig，不从通道 JSON 读。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects hostConfig as an unknown top-level key", "[unit][config][parse][CFG-reject-hostconfig-top]")
{
    const TempConfigFile file(
        std::string(R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )") +
        kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// igConfig 只写 udpRecvPort 则拒绝（多通道同步模块设计.md §3.1 方案 A，码 CFG-reject-partial-igconfig）。
// 本端端口和 Host 地址四个字段全必填。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects partial igConfig object (scheme A)", "[unit][config][parse][CFG-reject-partial-igconfig]")
{
    const TempConfigFile file(
        std::string(R"({ "igConfig": { "udpRecvPort": 8000 }, )") +
        kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// igConfig 缺 hostTcpPort 则拒绝（多通道同步模块设计.md §3.1，码 CFG-reject-igconfig-missing-target）。
// hostAddr 与 hostUdpRecvPort 已经写上，失败原因是缺目标 TCP，不是整段缺失。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects igConfig missing target fields (scheme A)", "[unit][config][parse][CFG-reject-igconfig-missing-target]")
{
    const TempConfigFile file(
        std::string(R"({ "igConfig": { "udpRecvPort": 8003, )") +
        R"("hostAddr": "127.0.0.1", "hostUdpRecvPort": 8000 }, )" + kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// igConfig 里的裸 tcpPort 是未知键，拒绝（多通道同步模块设计.md §3.1，码 CFG-reject-igconfig-tcpport）。
// 本端不设 TCP 端口；对端用 hostTcpPort。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects unknown igConfig.tcpPort (IgConfig has no bare tcpPort)",
          "[unit][config][parse][CFG-reject-igconfig-tcpport]")
{
    const TempConfigFile file(
        std::string(R"({ "igConfig": { "udpRecvPort": 8003, )") +
        R"("hostAddr": "127.0.0.1", "hostTcpPort": 8100, "hostUdpRecvPort": 8000, "tcpPort": 8100 }, )" +
        kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 旧键 igConfig.udpPortSend 拒绝（多通道同步模块设计.md §3.1，码 CFG-reject-udp-port-send）。
// 发送口已改成 hostUdpRecvPort，留下旧键不当作兼容。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects leftover igConfig.udpPortSend", "[unit][config][parse][CFG-reject-udp-port-send]")
{
    const TempConfigFile file(
        std::string(R"({ "igConfig": { "udpPortSend": 8000, "udpRecvPort": 8003, )") +
        R"("hostAddr": "127.0.0.1", "hostTcpPort": 8100, "hostUdpRecvPort": 8000 }, )" +
        kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 旧键 targetUdpPortSend 拒绝（多通道同步模块设计.md §3.1，码 CFG-reject-target-udp-port-send）。
// 对端 UDP 的键名是 hostUdpRecvPort。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects unknown igConfig.udpPortSend on target side (targetUdpPortSend)",
          "[unit][config][parse][CFG-reject-target-udp-port-send]")
{
    const TempConfigFile file(
        std::string(R"({ "igConfig": { "udpRecvPort": 8003, )") +
        R"("hostAddr": "127.0.0.1", "hostTcpPort": 8100, "hostUdpRecvPort": 8000, "targetUdpPortSend": 8001 }, )" +
        kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 只有 model 和 viewDefinition.window 的通道文件可以加载
// （多通道同步模块设计.md §3.1，码 CFG-accept-default-model-window）。
// 本例不调用 Engine::init。只钉 model 与窗口宽高，不钉 vfov。
// =============================================================================

TEST_CASE("loadEngineChannelConfig accepts default config with only model and window", "[unit][config][parse][CFG-accept-default-model-window]")
{
    const TempConfigFile file(kDefaultJson);
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.model == "models/lz.vsgt");
    REQUIRE(cfg.viewDefinition.window.width == 1920);
    REQUIRE(cfg.viewDefinition.window.height == 1080);
}


// =============================================================================
// 含 viewDefinition 的 IG 样例能加载（多通道同步模块设计.md §3.1，码 CFG-accept-ig-sample-syncsystem）。
// viewID 读入 ViewDefinition.viewID；不验收窗口像素。
// =============================================================================
TEST_CASE("loadEngineChannelConfig accepts IG-only sample config with viewDefinition", "[unit][config][parse][CFG-accept-ig-sample-syncsystem]")
{
    const TempConfigFile file(kMainJson);
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.igConfig.has_value());
    REQUIRE(cfg.igConfig->udpRecvPort == 8001);
    REQUIRE_FALSE(cfg.igConfig->hostAddr.empty());
    REQUIRE(cfg.viewDefinition.viewID == 0);
}

// =============================================================================
// 左通道样例能读出本端 UDP 与 Host 地址（多通道同步模块设计.md §3.1，码 CFG-accept-ig-sample）。
// 样例里 requireConnectedIg 是 false；本例不验这个字段，只验地址块。
// =============================================================================

TEST_CASE("loadEngineChannelConfig accepts IG-only sample config", "[unit][config][parse][CFG-accept-ig-sample]")
{
    const TempConfigFile file(kLeftJson);
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.igConfig->udpRecvPort == 8003);
    REQUIRE_FALSE(cfg.igConfig->hostAddr.empty());
}


// =============================================================================
// viewDefinition 整组可解析，JSON heading 取反后进入 OffsetDeg.yaw
// （sync模块化设计.md §4.2，码 CFG-parse-syncsystem）。
// heading -18.05 对应内部 yaw +18.05；pitch/roll 不取反。
// =============================================================================
TEST_CASE("loadEngineChannelConfig parses viewDefinition group", "[unit][config][parse][syncSystem][CFG-parse-syncsystem]")
{
    const TempConfigFile file(
        R"({ "viewDefinition": { "viewID": 3, "offsetDeg": { "heading": -18.05, "pitch": 1.0, "roll": 2.0 }, )"
        R"("requireConnectedIg": true }, )"
        R"("injectEllipsoidIfMissing": true, )"
        R"("igConfig": { "udpRecvPort": 8003, )"
        R"("hostAddr": "127.0.0.1", "hostTcpPort": 8100, "hostUdpRecvPort": 8000 }, )"
        R"("model": "models/lz.vsgt" })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.viewDefinition.viewID == 3);
    REQUIRE(offsetEquals(cfg.viewDefinition.offsetDeg, OffsetDeg{18.05, 1.0, 2.0}));
    REQUIRE(cfg.viewDefinition.requireConnectedIg);
}


// =============================================================================
// viewDefinition 里出现 schema 未声明的键时拒绝加载
// （多通道同步模块设计.md §3.1，码 CFG-reject-unknown-syncsystem）。
// 旧键 syncSystem / channelId 同样是未知键；本例用 bogus 代表任意未声明键。
// =============================================================================
TEST_CASE("loadEngineChannelConfig rejects unknown key inside viewDefinition group", "[unit][config][parse][syncSystem][CFG-reject-unknown-syncsystem]")
{
    const TempConfigFile file(R"({ "viewDefinition": { "viewID": 0, "bogus": 1 }, "model": "models/lz.vsgt" })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 已删除的 hostEyeStalePolicy 再出现则拒绝
// （多通道同步模块设计.md §3.1，码 CFG-reject-host-eye-stale）。
// 不保留旧枚举的兼容读法。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects removed hostEyeStalePolicy key", "[unit][config][parse][syncSystem][CFG-reject-host-eye-stale]")
{
    const TempConfigFile file(
        R"({ "viewDefinition": { "viewID": 0, "hostEyeStalePolicy": "ReuseLast" }, "model": "models/lz.vsgt" })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}


// =============================================================================
// viewDefinition.offsetDeg 只写 heading、缺 pitch/roll 时拒绝加载
// （多通道同步模块设计.md §3.1 方案 A，码 CFG-reject-partial-syncsystem-offset）。
// 与顶层半填用例同一规则；不验未知键 yaw。
// =============================================================================
TEST_CASE("loadEngineChannelConfig rejects partial viewDefinition offsetDeg", "[unit][config][parse][syncSystem][CFG-reject-partial-syncsystem-offset]")
{
    const TempConfigFile file(R"({ "viewDefinition": { "offsetDeg": { "heading": 1.0 } }, "model": "models/lz.vsgt" })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 通道文件只记录实体目录路径，不内嵌实体数组
// （实体管理设计.md §4，码 CFG-parse-entities-file-path）。
// 本例不打开该路径，只验字符串原样留下。
// =============================================================================

TEST_CASE("loadEngineChannelConfig parses entitiesFilePath", "[unit][config][parse][entities-catalog][CFG-parse-entities-file-path]")
{
    const TempConfigFile file(std::string(R"({ "entitiesFilePath": "config/entities.json", )") + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.entitiesFilePath == "config/entities.json");
}

// =============================================================================
// 通道文件顶层写 entities 则拒绝（实体管理设计.md §4，码 CFG-reject-toplevel-entities）。
// 实体数组只允许出现在 entities.json。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects top-level entities key", "[unit][config][parse][entities-catalog][negative][CFG-reject-toplevel-entities]")
{
    const TempConfigFile file(std::string(R"({ "entities": [], )") + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 窗口整数写成 640.0 这种整数值浮点仍接受（多通道同步模块设计.md §3.1，码 CFG-accept-int-window）。
// 只抽查 posX 与 width；0.0 与 640.0 都应落成整数。
// =============================================================================

TEST_CASE("loadEngineChannelConfig accepts integer-valued floats for window ints",
          "[unit][config][parse][json-adapter][CFG-accept-int-window]")
{
    const TempConfigFile file(
        R"({ "viewDefinition": { "window": { "posX": 0.0, "posY": 0, "width": 640.0, "height": 480 } } })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.viewDefinition.window.posX == 0);
    REQUIRE(cfg.viewDefinition.window.width == 640);
    REQUIRE(cfg.viewDefinition.window.height == 480);
}

// =============================================================================
// 窗口宽度出现小数则拒绝（多通道同步模块设计.md §3.1，码 CFG-reject-fractional-window）。
// 错误文本含 integer，用来和未知键失败区分开。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects fractional window ints",
          "[unit][config][parse][json-adapter][negative][CFG-reject-fractional-window]")
{
    const TempConfigFile file(
        R"({ "viewDefinition": { "window": { "posX": 0, "posY": 0, "width": 640.5, "height": 480 } } })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(error.find("integer") != std::string::npos);
}

// =============================================================================
// 文件以 UTF-8 BOM 开头仍能解析（多通道同步模块设计.md §3.1，码 CFG-accept-utf8-bom）。
// 只验窗口宽度，确认 BOM 没有把根对象弄丢。
// =============================================================================

TEST_CASE("loadEngineChannelConfig accepts UTF-8 BOM", "[unit][config][parse][json-adapter][CFG-accept-utf8-bom]")
{
    const std::string body = std::string("\xEF\xBB\xBF{") + kMinimalModel + ", " + kMinimalWindow + "}";
    const TempConfigFile file(body);
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.viewDefinition.window.width == 640);
}

// =============================================================================
// JSON 尾随逗号拒绝（多通道同步模块设计.md §3.1，码 CFG-reject-trailing-comma）。
// 夹具仍用旧的顶层 window 键；失败可以是语法错误，不要求先报未知键。
// =============================================================================

TEST_CASE("loadEngineChannelConfig rejects trailing comma", "[unit][config][parse][json-adapter][negative][CFG-reject-trailing-comma]")
{
    const TempConfigFile file(R"({ "window": { "x": 0, "y": 0, "width": 640, "height": 480 }, })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 通道文件顶层写成单数 entity 则拒绝（位姿配置设计.md §1，码 CFG-reject-singular-entity）。
// 实体列表不进通道文件，应写到 entities.json 的 entities 数组。
// =============================================================================
TEST_CASE("loadEngineChannelConfig rejects singular entity key",
          "[unit][config][parse][pose][entities][CFG-reject-singular-entity]")
{
    const TempConfigFile file(std::string("{ \"entity\": { \"model\": \"models/lz.vsgt\" }, ") + kMinimalWindow + " }");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// camera 只写 ellipsoid 半轨可以解析（位姿配置设计.md §1，码 CFG-camera-pose-nonsel-half）。
// 运行时再按场景有没有椭球选半，解析阶段不强制某一半。
// =============================================================================
TEST_CASE("loadEngineChannelConfig accepts camera pose with only the non-selected half",
          "[unit][config][parse][pose][camera][CFG-camera-pose-nonsel-half]")
{
    const TempConfigFile file(std::string("{ ") + kMinimalWindow +
                              R"(, "camera": { "pose": { "ellipsoid": { "lla": { "lat": 39.9, "lon": 116.4, "alt": 500 }, )"
                              R"("eulerYprDeg": [0, 0, 0] } } } })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.camera.has_value());
    REQUIRE(cfg.camera->hasPoseEllipsoid);
    REQUIRE_FALSE(cfg.camera->hasPoseLocal);
}

// =============================================================================
// camera 的 local 姿态数组不是三个数则拒绝（位姿配置设计.md §1，码 CFG-camera-reject-incomplete-ypr）。
// position 是完整的三个数，失败来自 eulerYprDeg 只有两个。不钉错误文本。
// =============================================================================
TEST_CASE("loadEngineChannelConfig rejects camera local pose with incomplete eulerYprDeg",
          "[unit][config][parse][pose][camera][CFG-camera-reject-incomplete-ypr]")
{
    const TempConfigFile file(std::string("{ ") + kMinimalWindow +
                              R"(, "camera": { "pose": { "local": { "position": [0, -10, 5], "eulerYprDeg": [0, 0] } } } })");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}
