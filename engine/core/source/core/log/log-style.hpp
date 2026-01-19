#pragma once

#include "log-types.hpp"

#include <string>
#include <format>
#include <algorithm>
#include <type_traits>

namespace april
{
    /**
     * @brief A wrapper for a value with associated log styling.
     */
    template<typename T>
    struct StyledValue
    {
        T           value;
        ELogColor color{ELogColor::Default};
        ELogStyle style{ELogStyle::None};

        auto bold() -> StyledValue& { style |= ELogStyle::Bold; return *this; }
        auto italic() -> StyledValue& { style |= ELogStyle::Italic; return *this; }
        auto underline() -> StyledValue& { style |= ELogStyle::Underline; return *this; }

        auto red() -> StyledValue& { color = ELogColor::Red; return *this; }
        auto green() -> StyledValue& { color = ELogColor::Green; return *this; }
        auto blue() -> StyledValue& { color = ELogColor::Blue; return *this; }
        auto yellow() -> StyledValue& { color = ELogColor::Yellow; return *this; }
        auto cyan() -> StyledValue& { color = ELogColor::Cyan; return *this; }
        auto magenta() -> StyledValue& { color = ELogColor::Magenta; return *this; }
    };

    /**
     * @brief Helper to create a styled value.
     */
     // std::decay_t<T> will convert char[N] to const char*
    template<typename T>
    auto Styled(T&& value) -> StyledValue<std::decay_t<T>>
    {
        return {std::forward<T>(value)};
    }

} // namespace april

/**
 * @brief Formatter for StyledValue.
 */
template<typename T>
struct std::formatter<april::StyledValue<T>> : std::formatter<T>
{
    april::ELogColor overrideColor{april::ELogColor::Default};
    april::ELogStyle overrideStyle{april::ELogStyle::None};

    constexpr auto parse(std::format_parse_context& ctx)
    {
       auto it = ctx.begin();
        auto end = ctx.end();

        while (it != end && *it != '}')
        {
            if (*it == '<')
            {
                ++it;
                auto start = it;
                while (it != end && *it != '>' && *it != '}')
                {
                    ++it;
                }

                if (it != end && *it == '>')
                {
                    std::string_view tag(start, it);

                    if      (tag == "red")     overrideColor = april::ELogColor::Red;
                    else if (tag == "green")   overrideColor = april::ELogColor::Green;
                    else if (tag == "blue")    overrideColor = april::ELogColor::Blue;
                    else if (tag == "yellow")  overrideColor = april::ELogColor::Yellow;
                    else if (tag == "cyan")    overrideColor = april::ELogColor::Cyan;
                    else if (tag == "magenta") overrideColor = april::ELogColor::Magenta;
                    else if (tag == "white")   overrideColor = april::ELogColor::White;

                    else if (tag == "bold")      overrideStyle |= april::ELogStyle::Bold;
                    else if (tag == "italic")    overrideStyle |= april::ELogStyle::Italic;
                    else if (tag == "underline") overrideStyle |= april::ELogStyle::Underline;
                    else if (tag == "dim")       overrideStyle |= april::ELogStyle::Dim;
                    else if (tag == "blink")     overrideStyle |= april::ELogStyle::Blink;
                    else if (tag == "reverse")   overrideStyle |= april::ELogStyle::Reverse;

                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
        return it;
    }

    auto format(april::StyledValue<T> const& styled, std::format_context& ctx) const
    {
        april::ELogColor finalColor = styled.color;
        if (overrideColor != april::ELogColor::Default) {
            finalColor = overrideColor;
        }

        april::ELogStyle finalStyle = styled.style | overrideStyle;

        std::string result = "\033[";
        bool        first  = true;

        auto appendCode = [&](int code)
        {
            if (!first) result += ";";
            result += std::to_string(code);
            first = false;
        };

        if (enum_has_any_flags(finalStyle, april::ELogStyle::Bold)) appendCode(1);
        if (enum_has_any_flags(finalStyle, april::ELogStyle::Dim)) appendCode(2);
        if (enum_has_any_flags(finalStyle, april::ELogStyle::Italic)) appendCode(3);
        if (enum_has_any_flags(finalStyle, april::ELogStyle::Underline)) appendCode(4);
        if (enum_has_any_flags(finalStyle, april::ELogStyle::Blink)) appendCode(5);
        if (enum_has_any_flags(finalStyle, april::ELogStyle::Reverse)) appendCode(7);
        if (enum_has_any_flags(finalStyle, april::ELogStyle::Hidden)) appendCode(8);

        if (finalColor != april::ELogColor::Default)
        {
            appendCode(static_cast<int>(finalColor));
        }

        if (first) // No style applied
        {
            return std::formatter<T>::format(styled.value, ctx);
        }

        result += "m";

        auto out = std::copy(result.begin(), result.end(), ctx.out());

        std::formatter<T>::format(styled.value, ctx);
        out = std::format_to(out, "{}", styled.value);

        std::string reset = "\033[0m";
        return std::copy(reset.begin(), reset.end(), out);
    }
};