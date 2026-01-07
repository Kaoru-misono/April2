#pragma once

// `a` must be a power of two.

template <typename integral>
constexpr bool is_aligned(integral x, size_t a) noexcept
{
  return (x & (integral(a) - 1)) == 0;
}

template <typename integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
  return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

template <typename integral>
constexpr integral align_down(integral x, size_t a) noexcept
{
  return integral(x & ~integral(a - 1));
}