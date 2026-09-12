#pragma once

#include <nlohmann/json.hpp>

#include <initializer_list>
#include <string>

/// AeroVistaConfig：配置 JSON 的契约辅助（未知键拒绝、严格整数等）。
/// 语法解析用 nlohmann/json；不依赖 vsg / 引擎 / sync。
namespace aerovista::config
{
    /// 去掉 UTF-8 BOM 后按 RFC 8259 解析；语法错误抛 nlohmann::json::parse_error。
    nlohmann::json parseJsonText(std::string text);

    /// 在对象中查找键；不存在返回 nullptr。
    const nlohmann::json* find(const nlohmann::json& obj, const char* key);

    /// 拒绝某键的 JSON null 值（严格字段存在性）。
    void rejectNull(const nlohmann::json& v, const char* key);

    /// 拒绝任何不在 `allowed` 列表中的键。
    void rejectUnknownKeys(const nlohmann::json& obj, std::initializer_list<const char*> allowed);

    /// 要求键存在且非 null。
    const nlohmann::json& requireValue(const nlohmann::json& obj, const char* key);

    /// 要求键存在且其值为对象。
    const nlohmann::json& requireObjectValue(const nlohmann::json& obj, const char* key);

    /// 要求当前值是对象（`key` 仅用于错误信息）。
    const nlohmann::json& requireObject(const nlohmann::json& v, const char* key);

    /// 要求数字（整数或浮点）。
    double requireNumber(const nlohmann::json& obj, const char* key);

    /// 要求整数（`1.0` 接受，`1.5` 拒绝）。
    int requireInt(const nlohmann::json& obj, const char* key);

    /// 要求字符串。
    std::string requireString(const nlohmann::json& obj, const char* key);

    /// 要求布尔。
    bool requireBool(const nlohmann::json& obj, const char* key);
} // namespace aerovista::config
