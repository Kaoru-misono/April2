#pragma once

#include <core/math/type.hpp>
#include <nlohmann/json.hpp>

namespace nlohmann
{
    template<glm::length_t L, typename T, glm::qualifier Q>
    struct adl_serializer<glm::vec<L, T, Q>>
    {
        static auto to_json(json& j, glm::vec<L, T, Q> const& value) -> void
        {
            j = json::array();
            for (glm::length_t i = 0; i < L; ++i)
            {
                j.push_back(value[i]);
            }
        }

        static auto from_json(json const& j, glm::vec<L, T, Q>& value) -> void
        {
            for (glm::length_t i = 0; i < L; ++i)
            {
                j.at(i).get_to(value[i]);
            }
        }
    };

    template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
    struct adl_serializer<glm::mat<C, R, T, Q>>
    {
        static auto to_json(json& j, glm::mat<C, R, T, Q> const& value) -> void
        {
            j = json::array();
            for (glm::length_t c = 0; c < C; ++c)
            {
                auto column = json::array();
                for (glm::length_t r = 0; r < R; ++r)
                {
                    column.push_back(value[c][r]);
                }
                j.push_back(std::move(column));
            }
        }

        static auto from_json(json const& j, glm::mat<C, R, T, Q>& value) -> void
        {
            for (glm::length_t c = 0; c < C; ++c)
            {
                auto const& column = j.at(c);
                for (glm::length_t r = 0; r < R; ++r)
                {
                    column.at(r).get_to(value[c][r]);
                }
            }
        }
    };

    template<typename T, glm::qualifier Q>
    struct adl_serializer<glm::qua<T, Q>>
    {
        static auto to_json(json& j, glm::qua<T, Q> const& value) -> void
        {
            j = json::array({value.x, value.y, value.z, value.w});
        }

        static auto from_json(json const& j, glm::qua<T, Q>& value) -> void
        {
            j.at(0).get_to(value.x);
            j.at(1).get_to(value.y);
            j.at(2).get_to(value.z);
            j.at(3).get_to(value.w);
        }
    };
}
