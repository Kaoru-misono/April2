#pragma once

#include "profile-types.hpp"
#include <string>
#include <vector>

namespace april::core
{
    class ProfileExporter
    {
    public:
        /**
         * Exports profiling events to a Chrome Trace Event Format JSON file.
         * @param path The output file path.
         * @param events The events to export.
         */
        static auto exportToFile(std::string const& path, std::vector<ProfileEvent> const& events) -> void;
    };
}
