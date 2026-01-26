#include "profile-exporter.hpp"
#include <fstream>

namespace april::core
{
    auto ProfileExporter::exportToFile(std::string const& path, std::vector<ProfileEvent> const& events) -> void
    {
        // Skeleton implementation
        std::ofstream ofs(path);
        if (ofs.is_open())
        {
            ofs << "{ \"traceEvents\": [] }";
        }
    }
}

