// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <initializer_list>
#include <map>
#include <string>

namespace april::graphics
{
    class DefineList : public std::map<std::string, std::string>
    {
    public:
        DefineList() = default;
        DefineList(std::initializer_list<std::pair<const std::string, std::string>> initList) : std::map<std::string, std::string>(initList) {}

        /**
         * Adds a macro definition. If the macro already exists, it will be replaced.
         * @param[in] name The name of macro.
         * @param[in] value Optional. The value of the macro.
         * @return The updated list of macro definitions.
         */
        auto add(std::string const& name, std::string const& value = "") -> DefineList&
        {
            (*this)[name] = value;
            return *this;
        }

        /**
         * Removes a macro definition. If the macro doesn't exist, the call will be silently ignored.
         * @param[in] name The name of macro.
         * @return The updated list of macro definitions.
         */
        auto remove(std::string const& name) -> DefineList&
        {
            erase(name);
            return *this;
        }

        /**
         * Add a define list to the current list
         */
        auto add(DefineList const& defineList) -> DefineList&
        {
            for (auto const& pair : defineList)
            {
                add(pair.first, pair.second);
            }
            return *this;
        }

        /**
         * Remove a define list from the current list
         */
        auto remove(DefineList const& defineList) -> DefineList&
        {
            for (auto const& pair : defineList)
            {
                remove(pair.first);
            }
            return *this;
        }
    };
}
