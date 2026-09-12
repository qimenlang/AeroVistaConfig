#include <aerovista/config/ConfigJson.h>

#include <stdexcept>
#include <string>

namespace aerovista::config
{
    namespace
    {
        void stripUtf8Bom(std::string& text)
        {
            if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
                static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF)
            {
                text.erase(0, 3);
            }
        }

        bool isIntegralNumber(const nlohmann::json& v)
        {
            if (v.is_number_integer() || v.is_number_unsigned())
                return true;
            if (!v.is_number_float())
                return false;
            const double n = v.get<double>();
            return n == static_cast<double>(static_cast<long long>(n));
        }
    } // namespace

    nlohmann::json parseJsonText(std::string text)
    {
        stripUtf8Bom(text);
        return nlohmann::json::parse(text);
    }

    const nlohmann::json* find(const nlohmann::json& obj, const char* key)
    {
        const auto it = obj.find(key);
        return it == obj.end() ? nullptr : &(*it);
    }

    void rejectNull(const nlohmann::json& v, const char* key)
    {
        if (v.is_null())
            throw std::runtime_error(std::string("null is invalid: ") + key);
    }

    void rejectUnknownKeys(const nlohmann::json& obj, std::initializer_list<const char*> allowed)
    {
        for (const auto& item : obj.items())
        {
            const std::string& key = item.key();
            bool known = false;
            for (const char* a : allowed)
            {
                if (key == a)
                {
                    known = true;
                    break;
                }
            }
            if (!known)
                throw std::runtime_error("unknown key: " + key);
        }
    }

    const nlohmann::json& requireValue(const nlohmann::json& obj, const char* key)
    {
        const nlohmann::json* v = find(obj, key);
        if (!v)
            throw std::runtime_error(std::string("missing key: ") + key);
        rejectNull(*v, key);
        return *v;
    }

    const nlohmann::json& requireObject(const nlohmann::json& v, const char* key)
    {
        rejectNull(v, key);
        if (!v.is_object())
            throw std::runtime_error(std::string("expected object: ") + key);
        return v;
    }

    const nlohmann::json& requireObjectValue(const nlohmann::json& obj, const char* key)
    {
        return requireObject(requireValue(obj, key), key);
    }

    double requireNumber(const nlohmann::json& obj, const char* key)
    {
        const nlohmann::json& v = requireValue(obj, key);
        if (!v.is_number())
            throw std::runtime_error(std::string("expected number: ") + key);
        return v.get<double>();
    }

    int requireInt(const nlohmann::json& obj, const char* key)
    {
        const nlohmann::json& v = requireValue(obj, key);
        if (!v.is_number())
            throw std::runtime_error(std::string("expected number: ") + key);
        if (!isIntegralNumber(v))
            throw std::runtime_error(std::string("expected integer: ") + key);
        return static_cast<int>(v.get<double>());
    }

    std::string requireString(const nlohmann::json& obj, const char* key)
    {
        const nlohmann::json& v = requireValue(obj, key);
        if (!v.is_string())
            throw std::runtime_error(std::string("expected string: ") + key);
        return v.get<std::string>();
    }

    bool requireBool(const nlohmann::json& obj, const char* key)
    {
        const nlohmann::json& v = requireValue(obj, key);
        if (!v.is_boolean())
            throw std::runtime_error(std::string("expected bool: ") + key);
        return v.get<bool>();
    }
} // namespace aerovista::config
