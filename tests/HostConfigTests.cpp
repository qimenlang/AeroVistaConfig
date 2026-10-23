#include <catch2/catch_test_macros.hpp>

#include <aerovista/config/HostConfig.h>

#include "Testutil.h"

#include <string>

using aerovista::config::HostConfig;
using aerovista::config::IgConfig;
using aerovista::config::loadHostConfig;
using aerovista::config::loadIgConfig;

// =============================================================================
// 只含 hostConfig 的文件读出本端两个端口，中继保持关闭
// （sync模块化设计.md §7，码 CFG-host-parse-ok）。
// 不写 relay 时 enable 为 false，igConfig 无值。不验端口范围。
// =============================================================================
TEST_CASE("loadHostConfig parses a host-only config file", "[unit][config][sync][host][CFG-host-parse-ok]")
{
    const TempConfigFile file(R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE(cfg.udpPortRecv == 8000);
    REQUIRE(cfg.tcpPort == 8100);
    REQUIRE_FALSE(cfg.relay.enable);
    REQUIRE_FALSE(cfg.igConfig.has_value());
}

// =============================================================================
// Host 文件根上出现 hostConfig / relay / igConfig 以外的键则拒绝
// （sync模块化设计.md §7，码 CFG-host-reject-unknown）。
// 未知键用 bogus。只验失败且错误非空，不钉错误文本。
// =============================================================================
TEST_CASE("loadHostConfig rejects unknown top-level keys", "[unit][config][sync][host][CFG-host-reject-unknown]")
{
    const TempConfigFile file(R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, "bogus": 1 })");
    HostConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// hostConfig 只写 udpPortRecv、缺 tcpPort 则拒绝（sync模块化设计.md §7，码 CFG-host-reject-partial）。
// 方案 A：对象出现则子字段全必填。
// =============================================================================
TEST_CASE("loadHostConfig rejects partial hostConfig object", "[unit][config][sync][host][CFG-host-reject-partial]")
{
    const TempConfigFile file(R"({ "hostConfig": { "udpPortRecv": 8000 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// relay.enable 为 false 时，旁边的 expectedIgCount 和完整 igConfig 都不读
// （sync模块化设计.md §7，码 CFG-host-relay-off）。
// 虚 IG 地址写成 10.0.0.5，断言它没有进入 HostConfig。
// =============================================================================
TEST_CASE("loadHostConfig leaves relay off when enable is false even if igConfig is present",
          "[unit][config][sync][host][CFG-host-relay-off]")
{
    const TempConfigFile file(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": false, "expectedIgCount": 3 }, )"
        R"("igConfig": { "udpRecvPort": 8002, "hostAddr": "10.0.0.5", )"
        R"("hostTcpPort": 9100, "hostUdpRecvPort": 9000 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(cfg.relay.enable);
    REQUIRE_FALSE(cfg.igConfig.has_value());
}

// =============================================================================
// relay.enable 且台数≥1、igConfig 完整时，虚 IG 地址按 JSON 原样读出
// （sync模块化设计.md §7，码 CFG-host-relay-on）。
// 用 8002 / 10.0.0.5，确认没有被 hostConfig 的 8000 盖掉。
// =============================================================================
TEST_CASE("loadHostConfig reads virtual IG from igConfig when relay is enabled",
          "[unit][config][sync][host][CFG-host-relay-on]")
{
    const TempConfigFile file(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": true, "expectedIgCount": 3 }, )"
        R"("igConfig": { "udpRecvPort": 8002, "hostAddr": "10.0.0.5", )"
        R"("hostTcpPort": 9100, "hostUdpRecvPort": 9000 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE(cfg.relay.enable);
    REQUIRE(cfg.relay.expectedIgCount == 3);
    REQUIRE(cfg.igConfig.has_value());
    REQUIRE(cfg.igConfig->udpRecvPort == 8002);
    REQUIRE(cfg.igConfig->hostAddr == "10.0.0.5");
    REQUIRE(cfg.igConfig->hostTcpPort == 9100);
    REQUIRE(cfg.igConfig->hostUdpRecvPort == 9000);
}

// =============================================================================
// relay.enable 但没有完整 igConfig 则拒绝（sync模块化设计.md §7，码 CFG-host-relay-need-ig）。
// 缺整段，和只写 udpRecvPort，都算不完整。第二次调用前清掉 error，避免串到上一次。
// =============================================================================
TEST_CASE("loadHostConfig rejects relay enable without igConfig",
          "[unit][config][sync][host][CFG-host-relay-need-ig]")
{
    const TempConfigFile missing(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": true, "expectedIgCount": 1 } })");
    const TempConfigFile partial(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": true, "expectedIgCount": 1 }, )"
        R"("igConfig": { "udpRecvPort": 8002 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadHostConfig(missing.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
    error.clear();
    REQUIRE_FALSE(loadHostConfig(partial.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// relay.enable 且 igConfig 完整，只缺 expectedIgCount 则拒绝
// （sync模块化设计.md §7，码 CFG-host-relay-need-count）。
// =============================================================================
TEST_CASE("loadHostConfig rejects relay enable without expectedIgCount",
          "[unit][config][sync][host][CFG-host-relay-need-count]")
{
    const TempConfigFile file(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": true }, )"
        R"("igConfig": { "udpRecvPort": 8002, "hostAddr": "10.0.0.5", )"
        R"("hostTcpPort": 9100, "hostUdpRecvPort": 9000 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// expectedIgCount 为 0 或 1.5 都拒绝（sync模块化设计.md §7，码 CFG-host-relay-count）。
// 0 小于 1；1.5 不是整数。合法下界 1 不在本例。
// =============================================================================
TEST_CASE("loadHostConfig rejects relay expectedIgCount below one",
          "[unit][config][sync][host][CFG-host-relay-count]")
{
    const TempConfigFile zero(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": true, "expectedIgCount": 0 }, )"
        R"("igConfig": { "udpRecvPort": 8002, "hostAddr": "10.0.0.5", )"
        R"("hostTcpPort": 9100, "hostUdpRecvPort": 9000 } })");
    const TempConfigFile frac(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": true, "expectedIgCount": 1.5 }, )"
        R"("igConfig": { "udpRecvPort": 8002, "hostAddr": "10.0.0.5", )"
        R"("hostTcpPort": 9100, "hostUdpRecvPort": 9000 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadHostConfig(zero.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
    error.clear();
    REQUIRE_FALSE(loadHostConfig(frac.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// 虚 IG 的 udpRecvPort 与 Host udpPortRecv 相同则拒绝
// （sync模块化设计.md §7，码 CFG-host-relay-port）。
// 撞的是本端收包口，不是 igConfig.hostUdpRecvPort。
// =============================================================================
TEST_CASE("loadHostConfig rejects relay when virtual IG UDP port matches Host",
          "[unit][config][sync][host][CFG-host-relay-port]")
{
    const TempConfigFile file(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": true, "expectedIgCount": 1 }, )"
        R"("igConfig": { "udpRecvPort": 8000, "hostAddr": "10.0.0.5", )"
        R"("hostTcpPort": 9100, "hostUdpRecvPort": 9000 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// relay 里出现 enable / expectedIgCount 以外的键则拒绝
// （sync模块化设计.md §7，码 CFG-host-relay-unknown）。
// enable 仍是 false：中继关着也不跳过未知键检查。
// =============================================================================
TEST_CASE("loadHostConfig rejects unknown keys inside relay",
          "[unit][config][sync][host][CFG-host-relay-unknown]")
{
    const TempConfigFile file(
        R"({ "hostConfig": { "udpPortRecv": 8000, "tcpPort": 8100 }, )"
        R"("relay": { "enable": false, "bogus": 1 } })");
    HostConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadHostConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// IG 文件从根上的 igConfig 块读出本端收包口和 Host 地址
// （sync模块化设计.md §7，码 CFG-ig-parse-ok）。
// 根不是裸端口字段。端口用 8005，与 Host 样例错开。
// =============================================================================
TEST_CASE("loadIgConfig parses an ig-only config file", "[unit][config][sync][ig][CFG-ig-parse-ok]")
{
    const TempConfigFile file(
        R"({ "igConfig": { "udpRecvPort": 8005, )"
        R"("hostAddr": "127.0.0.1", "hostTcpPort": 8100, "hostUdpRecvPort": 8000 } })");
    IgConfig cfg;
    std::string error;
    REQUIRE(loadIgConfig(file.path(), cfg, &error));
    REQUIRE(cfg.udpRecvPort == 8005);
    REQUIRE(cfg.hostAddr == "127.0.0.1");
    REQUIRE(cfg.hostTcpPort == 8100);
    REQUIRE(cfg.hostUdpRecvPort == 8000);
}

// =============================================================================
// IG 文件根上多出 hostConfig 则拒绝（sync模块化设计.md §7，码 CFG-ig-reject-unknown）。
// IG 文件只允许 igConfig 这一层，不嵌 Host 块。
// =============================================================================
TEST_CASE("loadIgConfig rejects unknown top-level keys", "[unit][config][sync][ig][CFG-ig-reject-unknown]")
{
    const TempConfigFile file(
        R"({ "igConfig": { "udpRecvPort": 8005, )"
        R"("hostAddr": "127.0.0.1", "hostTcpPort": 8100, "hostUdpRecvPort": 8000 }, )"
        R"("hostConfig": {} })");
    IgConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadIgConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}

// =============================================================================
// igConfig 缺 hostUdpRecvPort、其余三个字段都在时拒绝
// （sync模块化设计.md §7，码 CFG-ig-reject-partial）。
// 失败原因是半填，不是整段缺失。
// =============================================================================
TEST_CASE("loadIgConfig rejects partial igConfig object", "[unit][config][sync][ig][CFG-ig-reject-partial]")
{
    const TempConfigFile file(
        R"({ "igConfig": { "udpRecvPort": 8005, )"
        R"("hostAddr": "127.0.0.1", "hostTcpPort": 8100 } })");
    IgConfig cfg;
    std::string error;
    REQUIRE_FALSE(loadIgConfig(file.path(), cfg, &error));
    REQUIRE_FALSE(error.empty());
}
