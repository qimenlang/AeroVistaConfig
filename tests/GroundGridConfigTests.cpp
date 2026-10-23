#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <aerovista/config/EngineChannelConfig.h>

#include "Testutil.h"

#include <string>

using aerovista::config::EngineChannelConfig;
using aerovista::config::loadEngineChannelConfig;

namespace
{
    const char* kMinimalWindow =
        R"("viewDefinition": { "vfov": 30.0, "near": 1.0, "far": 1000.0, "window": { "posX": 0, "posY": 0, "width": 640, "height": 480 } })";
    const char* kMinimalModel = R"("model": "models/lz.vsgt")";
    const char* kGridLlaJson = R"("lla": { "lat": 39.9087, "lon": 116.3975, "alt": 0.0 })";
} // namespace

// =============================================================================
// 只给 lla 时，半边长、单格边长、主线间隔走缺省
// （位姿配置设计.md §1，码 MEAS-seam-grid）。
// 缺省是契约：20000 m / 200 m / 5，不是把样例 JSON 整份拿来比对。
// =============================================================================
TEST_CASE("loadEngineChannelConfig parses groundGrid with defaults",
          "[unit][sync][meas][MEAS-seam-grid]")
{
    const TempConfigFile file(std::string(R"({ "groundGrid": { )") + kGridLlaJson + " }, " + kMinimalModel + ", " +
                              kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE(cfg.groundGrid);
    REQUIRE(cfg.groundGrid->lla.x == Catch::Approx(39.9087));
    REQUIRE(cfg.groundGrid->lla.y == Catch::Approx(116.3975));
    REQUIRE(cfg.groundGrid->lla.z == Catch::Approx(0.0));
    REQUIRE(cfg.groundGrid->halfGroundLenM == Catch::Approx(20000.0));
    REQUIRE(cfg.groundGrid->gridLenM == Catch::Approx(200.0));
    REQUIRE(cfg.groundGrid->majorLineEvery == 5);
}

// =============================================================================
// groundGrid 缺 lla 时加载失败（位姿配置设计.md §1，码 MEAS-seam-grid）。
// 对象里另写了合法的 gridLenM，失败原因应是缺 lla，而不是未知键。
// =============================================================================
TEST_CASE("loadEngineChannelConfig rejects groundGrid without lla",
          "[unit][sync][meas][MEAS-seam-grid]")
{
    const TempConfigFile file(std::string(R"({ "groundGrid": { "gridLenM": 100 }, )") + kMinimalModel + ", " +
                              kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_THAT(error, Catch::Matchers::ContainsSubstring("lla"));
}

// =============================================================================
// gridLenM 为 0 时加载失败（位姿配置设计.md §1，码 MEAS-seam-grid）。
// 错误文本带字段名，调用方能对上是哪一项不合法。
// =============================================================================
TEST_CASE("loadEngineChannelConfig rejects non-positive groundGrid.gridLenM",
          "[unit][sync][meas][MEAS-seam-grid]")
{
    const TempConfigFile file(std::string(R"({ "groundGrid": { )") + kGridLlaJson + R"(, "gridLenM": 0 }, )" +
                              kMinimalModel + ", " + kMinimalWindow + "}");
    EngineChannelConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadEngineChannelConfig(file.path(), cfg, &error));
    REQUIRE_THAT(error, Catch::Matchers::ContainsSubstring("gridLenM"));
}
