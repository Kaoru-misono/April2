#include "profile-exporter.hpp"
#include "profile-manager.hpp"
#include <fstream>
#include <format>
#include <iostream>

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
        for (auto const& event : events)
        {
            ofs << ",\n";

            char const ph = (event.type == ProfileEventType::Begin) ? 'B' : 'E';
            uint64_t const tsUs = event.timestamp / 1000;

            // Standard Event Format: { "name": "...", "cat": "PERF", "ph": "...", "ts": ..., "pid": 0, "tid": ... }
            ofs << std::format("    {{ \"name\": \"{}\", \"cat\": \"PERF\", \"ph\": \"{}\", \"ts\": {}, \"pid\": 0, \"tid\": {} }}",
                event.name ? event.name : "Unknown", ph, tsUs, event.threadId);
        }

        ofs << "\n  ]\n}";
        ofs.close();
    }
}

