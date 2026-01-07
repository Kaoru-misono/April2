#pragma once

#include "idiom.hpp"
#include "../log/logger.hpp"

#include <thread>

namespace april
{
    template <typename Instance>
    struct ThreadExclusive
    {
        AP_PINNED(ThreadExclusive);

        inline static thread_local Instance* optInstance{};

        ThreadExclusive();
        ~ThreadExclusive();

        static auto current() -> Instance*;
        static auto tryCurrent() -> Instance*;
        static auto resetCurrent(Instance* optOther = {}) -> void;
    };

}

namespace april
{
    template <typename Instance>
    ThreadExclusive<Instance>::ThreadExclusive()
    {
        if (optInstance)
        {
            AP_ERROR("{} is thread exclusive, can not construct because there is already an instance.", typeid(Instance).name());
        }
        else
        {
            optInstance = (Instance*) this;
        }
    }

    template <typename Instance>
    ThreadExclusive<Instance>::~ThreadExclusive()
    {
        if (optInstance == (Instance*) this) optInstance = {};
    }

    template <typename Instance>
    auto ThreadExclusive<Instance>::current() -> Instance*
    {
        if (auto instance = optInstance)
        {
            return instance;
        }
        else
        {
            AP_ERROR("{} has no instance in called thread {}, create it first.", typeid(Instance).name(), std::this_thread::get_id());
            std::abort();
        }
    }

    template <typename Instance>
    auto ThreadExclusive<Instance>::tryCurrent() -> Instance*
    {
        return optInstance;
    }

    template <typename Instance>
    auto ThreadExclusive<Instance>::resetCurrent(Instance* optOther) -> void
    {
        optInstance = optOther;
    }
}
