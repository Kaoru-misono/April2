#include "profile-exporter.hpp"
#include "profile-manager.hpp"
#include "core/log/logger.hpp"
#include <fstream>
#include <format>
#include <iostream>
#include <limits>

namespace april::core
{
    auto ProfileExporter::exportToFile(std::string const& path, std::vector<ProfileEvent> const& events) -> void
    {
        std::ofstream ofs(path);
        if (!ofs.is_open())
        {
            return;
        }

        ofs << "{\n  \"traceEvents\": [\n";

        // 1. Write Metadata: Process Name
        ofs << "    { \"name\": \"process_name\", \"ph\": \"M\", \"pid\": 0, \"args\": { \"name\": \"April Engine\" } }";

        // 2. Write Metadata: Thread Names
        auto const& threadNames = ProfileManager::get().getThreadNames();
        for (auto const& [tid, name] : threadNames)
        {
            ofs << ",\n";
            ofs << std::format("    {{ \"name\": \"thread_name\", \"ph\": \"M\", \"pid\": 0, \"tid\": {}, \"args\": {{ \"name\": \"{}\" }} }}",
                tid, name);
        }

        // 3. Write Events
        constexpr double kInvalidTimestamp = static_cast<double>(std::numeric_limits<int64_t>::max());
        for (auto const& event : events)
        {
            if (event.timestamp == 0.0 || event.timestamp >= kInvalidTimestamp)
            {
                AP_WARN("Filtered event: {} ts={}", event.name ? event.name : "Unknown", event.timestamp);
                continue;
            }
            ofs << ",\n";

            if (event.type == ProfileEventType::Instant)
            {
                ofs << std::format("    {{ \"name\": \"{}\", \"cat\": \"PERF\", \"ph\": \"i\", \"ts\": {}, \"pid\": 0, \"tid\": {} }}",
                    event.name ? event.name : "Unknown", event.timestamp, event.threadId);
            }
            else
            {
                ofs << std::format("    {{ \"name\": \"{}\", \"cat\": \"PERF\", \"ph\": \"X\", \"ts\": {}, \"dur\": {}, \"pid\": 0, \"tid\": {} }}",
                    event.name ? event.name : "Unknown", event.timestamp, event.duration, event.threadId);
            }
        }

        ofs << "\n  ]\n}";
        ofs.close();
    }
}
