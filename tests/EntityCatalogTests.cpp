#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <aerovista/config/EngineChannelConfig.h>
#include <aerovista/config/EntityCatalog.h>

#include "Testutil.h"

#include <cstdint>
#include <string>
#include <vector>

using aerovista::config::EntityAuthorityRow;
using aerovista::config::EntityAuthorityState;
using aerovista::config::EntityConfig;
using aerovista::config::EntityInitialState;
using aerovista::config::loadEntitiesFile;
using aerovista::config::loadHostEntityCatalog;


// =============================================================================
// 两条实体进目录：缺省 Active、显式 Standby，local 半轨读进 position 与 euler
// （实体管理设计.md §4，码 CFG-entities-parse）。
// 第一条不写 pose，hasPose 为 false。第二条只抽查 position.x 与 euler.y，不把六个数都钉死。
// =============================================================================

TEST_CASE("loadEntitiesFile parses entities.json into EntityConfig list", "[unit][config][parse][entities-catalog][CFG-entities-parse]")
{
    const TempConfigFile file(
        R"({ "entities": [)"
        R"({ "id": 1, "name": "teapot_center", "model": "models/teapot.vsgt" },)"
        R"({ "id": 2, "name": "truck", "model": "models/truck.vsgt", "initialEntityState": "Standby", )"
        R"("pose": { "local": { "position": [1, 2, 3], "eulerYprDeg": [10, 20, 30] } } })"
        R"(] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE(loadEntitiesFile(file.path(), entities, &error));

    THEN("both entities parsed with defaults applied")
    {
        REQUIRE(entities.size() == 2);
        REQUIRE(entities[0].id == 1);
        REQUIRE(entities[0].name == "teapot_center");
        REQUIRE(entities[0].model == "models/teapot.vsgt");
        REQUIRE(entities[0].initialEntityState == EntityInitialState::ACTIVE); // 缺省 Active
        REQUIRE_FALSE(entities[0].hasPose);

        REQUIRE(entities[1].id == 2);
        REQUIRE(entities[1].name == "truck");
        REQUIRE(entities[1].initialEntityState == EntityInitialState::STANDBY);
        REQUIRE(entities[1].hasPose);
        REQUIRE(entities[1].hasPoseLocal);
        REQUIRE_FALSE(entities[1].hasPoseEllipsoid);
        REQUIRE(entities[1].localPose.position.x == Catch::Approx(1.0));
        REQUIRE(entities[1].localPose.eulerYprDeg.y == Catch::Approx(20.0));
    }
}

// =============================================================================
// 条目不写 name 时用 model 的文件名（实体管理设计.md §4，码 CFG-entities-name-default）。
// basename 保留扩展名，结果是 lz.vsgt，不是 lz。
// =============================================================================

TEST_CASE("loadEntitiesFile applies name default from model basename", "[unit][config][parse][entities-catalog][CFG-entities-name-default]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 7, "model": "models/lz.vsgt" } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE(entities.size() == 1);
    REQUIRE(entities[0].name == "lz.vsgt");
}

// =============================================================================
// initialEntityState 不是 Active 或 Standby 则拒绝（实体管理设计.md §4，码 CFG-entities-reject-state）。
// 小写 active、全大写 ACTIVE，以及 Destroyed / Remove / bogus 都算非法。不钉错误文本。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects invalid initialEntityState", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-state]")
{
    for (const char* bad : {R"("active")", R"("ACTIVE")", R"("Destroyed")", R"("Remove")", R"("bogus")"})
    {
        const std::string json = "{ \"entities\": [ { \"id\": 1, \"model\": \"models/lz.vsgt\", \"initialEntityState\": " +
                                 std::string(bad) + " } ] }";
        const TempConfigFile file(json);
        std::vector<EntityConfig> entities;
        std::string error;
        REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
        REQUIRE_FALSE(error.empty());
    }
}

// =============================================================================
// 两条实体字段都合法，只因 id 相同而拒绝（实体管理设计.md §4，码 CFG-entities-reject-dup）。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects duplicate entity id", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-dup]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/lz.vsgt" }, { "id": 1, "model": "models/teapot.vsgt" } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// id 不在 1..65535 则拒绝（实体管理设计.md §4，码 CFG-entities-reject-id）。
// 覆盖 0、-1、65536、70000。下界 1 和上界 65535 不在本例。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects entity id outside 1..65535", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-id]")
{
    for (const char* badId : {"0", "-1", "65536", "70000"})
    {
        const std::string json = "{ \"entities\": [ { \"id\": " + std::string(badId) +
                                 ", \"model\": \"models/lz.vsgt\" } ] }";
        const TempConfigFile file(json);
        std::vector<EntityConfig> entities;
        std::string error;
        REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
        REQUIRE_FALSE(error.empty());
    }
}

// =============================================================================
// 缺 model，以及 model 为空字符串，都拒绝（实体管理设计.md §4，码 CFG-entities-reject-empty-model）。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects missing or empty model", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-empty-model]")
{
    for (const char* item : {R"({ "id": 1 })", R"({ "id": 1, "model": "" })"})
    {
        const std::string json = "{ \"entities\": [ " + std::string(item) + " ] }";
        const TempConfigFile file(json);
        std::vector<EntityConfig> entities;
        std::string error;
        REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
        REQUIRE_FALSE(error.empty());
    }
}

// =============================================================================
// entities 空数组拒绝（实体管理设计.md §4，码 CFG-entities-reject-empty-array）。
// 这和「文件里没有 entities 键」是两回事，后者另有码。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects empty entities array", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-empty-array]")
{
    const TempConfigFile file(R"({ "entities": [] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 文件根上或条目里出现未声明键则拒绝（实体管理设计.md §4，码 CFG-entities-reject-unknown-keys）。
// 两处各放一个 bogus，确认未知键规则不只套在根上。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects unknown keys in file or item", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-unknown-keys]")
{
    for (const char* json : {
             R"({ "entities": [ { "id": 1, "model": "models/lz.vsgt" } ], "bogus": 1 })",
             R"({ "entities": [ { "id": 1, "model": "models/lz.vsgt", "bogus": 1 } ] })"})
    {
        const TempConfigFile file(json);
        std::vector<EntityConfig> entities;
        std::string error;
        REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
        REQUIRE_FALSE(error.empty());
    }
}

// =============================================================================
// 没有 entities 键，或路径打不开，都拒绝（实体管理设计.md §4，码 CFG-entities-reject-missing-key）。
// 不区分这两种失败的错误文本。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects missing entities key or file", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-missing-key]")
{
    const TempConfigFile fileNoKey(R"({ "model": "models/lz.vsgt" })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE_FALSE(loadEntitiesFile(fileNoKey.path(), entities, &error));
    REQUIRE_FALSE(error.empty());

    REQUIRE_FALSE(loadEntitiesFile("nonexistent-entities.json", entities, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 条目有 model、没有 id 则拒绝（实体管理设计.md §4，码 CFG-entities-reject-missing-id）。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects item without id", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-missing-id]")
{
    const TempConfigFile file(R"({ "entities": [ { "model": "models/teapot.vsgt" } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// id 写成字符串则拒绝（实体管理设计.md §4，码 CFG-entities-reject-nonint-id）。
// 整数值浮点 1.0 另见 CFG-entities-accept-float-id，不在本例。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects item with non-integer id", "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-nonint-id]")
{
    const TempConfigFile file(R"({ "entities": [ { "id": "a", "model": "models/teapot.vsgt" } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// local 的 position 不是三个数则拒绝（实体管理设计.md §4，码 CFG-entities-reject-incomplete-pose）。
// eulerYprDeg 是完整的，失败来自 position 只有两个元素。
// =============================================================================

TEST_CASE("loadEntitiesFile rejects local pose with incomplete position array",
          "[unit][config][parse][entities-catalog][negative][CFG-entities-reject-incomplete-pose]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/teapot.vsgt", )"
        R"("pose": { "local": { "position": [1, 2], "eulerYprDeg": [0, 0, 0] } } } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE_FALSE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// id 写成 1.0 这种整数值浮点时收成整数 1（实体管理设计.md §4，码 CFG-entities-accept-float-id）。
// 1.5 这种带小数的值不在本例。
// =============================================================================

TEST_CASE("loadEntitiesFile accepts integer-valued float id", "[unit][config][parse][json-adapter][entities-catalog][CFG-entities-accept-float-id]")
{
    const TempConfigFile file(R"({ "entities": [ { "id": 1.0, "model": "models/teapot.vsgt" } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE(entities.size() == 1);
    REQUIRE(entities[0].id == 1);
}

// =============================================================================
// 只写 ellipsoid 半轨也能进实体目录（位姿配置设计.md §1，码 CFG-entities-pose-nonsel-half）。
// 不再按 injectEllipsoidIfMissing 强制某一半。不验 lla 数值，只验哪一半被置位。
// =============================================================================
TEST_CASE("loadEntitiesFile accepts pose with only the non-selected half",
          "[unit][config][parse][pose][entities-catalog][CFG-entities-pose-nonsel-half]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/teapot.vsgt", )"
        R"("pose": { "ellipsoid": { "lla": { "lat": 39.9, "lon": 116.4, "alt": 0 }, )"
        R"("eulerYprDeg": [0, 0, 0] } } } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE(entities.size() == 1);
    REQUIRE(entities[0].hasPoseEllipsoid);
    REQUIRE_FALSE(entities[0].hasPoseLocal);
}

// =============================================================================
// 只写 local 半轨也能进实体目录（位姿配置设计.md §1，码 CFG-entities-pose-no-ellipsoid）。
// 全 0 的 position 与 euler 也算写了 local。不要求同时有 ellipsoid。
// =============================================================================
TEST_CASE("loadEntitiesFile accepts pose with ellipsoid half absent",
          "[unit][config][parse][pose][entities-catalog][CFG-entities-pose-no-ellipsoid]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/teapot.vsgt", )"
        R"("pose": { "local": { "position": [0, 0, 0], "eulerYprDeg": [0, 0, 0] } } } ] })");
    std::vector<EntityConfig> entities;
    std::string error;
    REQUIRE(loadEntitiesFile(file.path(), entities, &error));
    REQUIRE(entities.size() == 1);
    REQUIRE(entities[0].hasPoseLocal);
    REQUIRE_FALSE(entities[0].hasPoseEllipsoid);
}

// =============================================================================
// Host 子集按 initialEntityState 给出两行（实体与运动控制设计.md §11，码 ENT-04-table-init）。
// 直接断言 loadHostEntityCatalog 的行，不经过 HostDataManager。
// 缺省 Active、显式 Standby；alpha 255 与全 0 位姿是缺省，JSON 里没写。
// =============================================================================
TEST_CASE("loadHostEntityCatalog builds entity rows from initialEntityState",
          "[unit][sync][host-data][ENT-04-table-init]")
{
    const TempConfigFile file(
        R"({ "entities": [)"
        R"({ "id": 1, "name": "teapot_center", "model": "models/teapot.vsgt" },)"
        R"({ "id": 2, "name": "truck", "model": "models/truck.vsgt", "initialEntityState": "Standby" })"
        R"(] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(error.empty());
    REQUIRE(rows.size() == 2);

    const EntityAuthorityRow* first = findRow(rows, 1);
    REQUIRE(first != nullptr);
    REQUIRE(first->name == "teapot_center");
    REQUIRE(first->model == "models/teapot.vsgt");
    REQUIRE(first->entityState == EntityAuthorityState::ACTIVE);
    REQUIRE(first->alpha == 255);
    REQUIRE(poseIsZero(first->pose));

    const EntityAuthorityRow* second = findRow(rows, 2);
    REQUIRE(second != nullptr);
    REQUIRE(second->entityState == EntityAuthorityState::STANDBY);
    REQUIRE(second->alpha == 255);
    REQUIRE(poseIsZero(second->pose));
}

// =============================================================================
// 子集不写 name 时用 model 的文件名（实体与运动控制设计.md §11，码 ENT-04-table-name）。
// basename 保留扩展名，结果是 lz.vsgt。不经过 HostDataManager。
// =============================================================================
TEST_CASE("loadHostEntityCatalog defaults entity name from model basename",
          "[unit][sync][host-data][ENT-04-table-name]")
{
    const TempConfigFile file(R"({ "entities": [ { "id": 7, "model": "models/lz.vsgt" } ] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].name == "lz.vsgt");
}

// =============================================================================
// 子集只把 pose.ellipsoid 写入权威位姿（实体与运动控制设计.md §11，码 ENT-04-table-pose）。
// eulerYprDeg 按 yaw、pitch、roll 对应数组的三个数。不验 local 半轨。
// =============================================================================
TEST_CASE("loadHostEntityCatalog writes ellipsoid pose into the entity row",
          "[unit][sync][host-data][ENT-04-table-pose]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/teapot.vsgt", )"
        R"("pose": { "ellipsoid": { "lla": { "lat": 1.0, "lon": 2.0, "alt": 3.0 }, )"
        R"("eulerYprDeg": [10.0, 20.0, 30.0] } } } ] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].entityState == EntityAuthorityState::ACTIVE);
    REQUIRE(rows[0].pose.lat == 1.0);
    REQUIRE(rows[0].pose.lon == 2.0);
    REQUIRE(rows[0].pose.alt == 3.0);
    REQUIRE(rows[0].pose.yawDeg == 10.0);
    REQUIRE(rows[0].pose.pitchDeg == 20.0);
    REQUIRE(rows[0].pose.rollDeg == 30.0);
}

// =============================================================================
// 子集只有 pose.local 时权威位姿保持全 0（实体与运动控制设计.md §11，码 ENT-04-table-pose-local）。
// local 的 1,2,3 与 4,5,6 不得漏进 EntityAuthorityPose。对象本身允许出现。
// =============================================================================
TEST_CASE("loadHostEntityCatalog leaves pose at zero when only local pose is present",
          "[unit][sync][host-data][ENT-04-table-pose-local]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/teapot.vsgt", )"
        R"("pose": { "local": { "position": [1.0, 2.0, 3.0], "eulerYprDeg": [4.0, 5.0, 6.0] } } } ] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(rows.size() == 1);
    REQUIRE(poseIsZero(rows[0].pose));
}

// =============================================================================
// initialEntityState 写成 Destroyed 则拒绝（实体与运动控制设计.md §11，码 ENT-04-table-reject-state）。
// 只钉错误文本含 initialEntityState，不钉整句。不经过 HostDataManager。
// =============================================================================
TEST_CASE("loadHostEntityCatalog rejects invalid initialEntityState",
          "[unit][sync][host-data][negative][ENT-04-table-reject-state]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/lz.vsgt", "initialEntityState": "Destroyed" } ] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE_FALSE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(error.find("initialEntityState") != std::string::npos);
}

// =============================================================================
// 子集里重复 id 拒绝（实体与运动控制设计.md §11，码 ENT-04-table-reject-dup）。
// 只钉错误文本含 duplicate。
// =============================================================================
TEST_CASE("loadHostEntityCatalog rejects duplicate entity id",
          "[unit][sync][host-data][negative][ENT-04-table-reject-dup]")
{
    const TempConfigFile file(
        R"({ "entities": [ { "id": 1, "model": "models/lz.vsgt" }, { "id": 1, "model": "models/teapot.vsgt" } ] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE_FALSE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(error.find("duplicate") != std::string::npos);
}

// =============================================================================
// id 为 0 则拒绝（实体与运动控制设计.md §11，码 ENT-04-table-reject-id）。
// 只钉错误文本含 1..65535。上界 65536 不在本例。
// =============================================================================
TEST_CASE("loadHostEntityCatalog rejects entity id outside 1..65535",
          "[unit][sync][host-data][negative][ENT-04-table-reject-id]")
{
    const TempConfigFile file(R"({ "entities": [ { "id": 0, "model": "models/lz.vsgt" } ] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE_FALSE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(error.find("1..65535") != std::string::npos);
}

// =============================================================================
// entities 空数组拒绝（实体与运动控制设计.md §11，码 ENT-04-table-reject-empty）。
// 只钉错误文本含 empty。这和「文件打不开」不是同一种失败。
// =============================================================================
TEST_CASE("loadHostEntityCatalog rejects empty entities array",
          "[unit][sync][host-data][negative][ENT-04-table-reject-empty]")
{
    const TempConfigFile file(R"({ "entities": [] })");
    std::vector<EntityAuthorityRow> rows;
    std::string error;
    REQUIRE_FALSE(loadHostEntityCatalog(file.path(), rows, &error));
    REQUIRE(error.find("empty") != std::string::npos);
}
