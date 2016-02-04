/*-
* Copyright (c) 2013 Zhihao Yuan.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#pragma once

#ifndef _STDEX_RANDOM_H
#define _STDEX_RANDOM_H 1

#include <random>
#include <type_traits>

namespace std {

    namespace detail {

        inline auto get_seed()
            -> std::default_random_engine::result_type
        {
            std::default_random_engine::result_type lo{};
#if defined(__i386__) || defined(__x86_64__)
            __asm__ __volatile__("rdtsc" : "=a"(lo));
#endif
            return lo;
        }

        inline auto global_rng()
            -> std::default_random_engine&
        {
            thread_local std::default_random_engine e{ get_seed() };
            return e;
        }

    }

    template <typename IntType>
    inline IntType randint(IntType a, IntType b)
    {
        // does not entirely satisfy 26.5.1.1/1(e).
        static_assert(std::is_integral<IntType>(), "not an integral");

        using distribution_type = std::uniform_int_distribution<IntType>;
        using param_type = typename distribution_type::param_type;

        thread_local distribution_type d;
        return d(detail::global_rng(), param_type(a, b));
    }

    inline void reseed()
    {
        // as far as uniform_int_distribution carries no state
        detail::global_rng().seed(detail::get_seed());
    }

    inline void reseed(std::default_random_engine::result_type value)
    {
        // as far as uniform_int_distribution carries no state
        detail::global_rng().seed(value);
    }

}

#endif

