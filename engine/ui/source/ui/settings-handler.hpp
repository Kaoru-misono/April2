#pragma once

#include <functional>
#include <string>
#include <sstream>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace april::ui
{
    // Example
    // m_settingsHandler.setHandlerName("MyHandlerName");
    // m_settingsHandler.setSetting("ShowLog", &m_showLog);
    // m_settingsHandler.setSetting("LogLevel", &m_logger);
    // m_settingsHandler.addImGuiHandler();
    class SettingsHandler
    {
    public:
        SettingsHandler();
        explicit SettingsHandler(const std::string& name);

        auto setHandlerName(const std::string& name) -> void { handlerName = name; }
        auto addImGuiHandler() -> void;

        template <typename T>
        auto setSetting(const std::string& key, T* value) -> void;

        private:
        struct SettingEntry
        {
            void*                                   ptr{};
            std::function<void(const std::string&)> fromString;
            std::function<std::string()>            toString;
        };

        std::string                                   handlerName{};
        std::unordered_map<std::string, SettingEntry> settings{};
    };

    // Default setting for int, float, double, vec2, vec3, bool, ...
    template <typename T>
    auto SettingsHandler::setSetting(const std::string& key, T* value) -> void
    {
        SettingEntry entry{.ptr = value};
        // value=23.3,45.2
        if constexpr(std::is_same_v<T, glm::ivec2> || std::is_same_v<T, glm::uvec2> || std::is_same_v<T, glm::vec2>)
        {
            entry.fromString = [value](const std::string& str) {
            std::stringstream ss(str);
            char              comma;
            ss >> value->x >> comma >> value->y;
            };
            entry.toString = [value]() { return std::to_string(value->x) + "," + std::to_string(value->y); };
        }
        // value=2.1,4.3,6.5
        else if constexpr(std::is_same_v<T, glm::ivec3> || std::is_same_v<T, glm::uvec3> || std::is_same_v<T, glm::vec3>)
        {
            entry.fromString = [value](const std::string& str) {
            std::stringstream ss(str);
            char              comma;
            ss >> value->x >> comma >> value->y >> comma >> value->z;
            };
            entry.toString = [value]() {
            return std::to_string(value->x) + "," + std::to_string(value->y) + "," + std::to_string(value->z);
            };
        }
        // value=true or value=false
        else if constexpr(std::is_same_v<T, bool>)
        {
            entry.fromString = [value](const std::string& str) { *value = (str == "true"); };
            entry.toString   = [value]() { return (*value) ? "true" : "false"; };
        }
        // All other type
        else
        {
            entry.fromString = [value](const std::string& str) {
            std::istringstream iss(str);
            iss >> *value;
            };
            entry.toString = [value]() {
            std::ostringstream oss;
            oss << *value;
            return oss.str();
            };
        }
        settings[key] = std::move(entry);
    }


}  // namespace april::ui
