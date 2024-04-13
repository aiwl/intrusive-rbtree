#pragma once

#include <functional>
#include <string>
#include <random>
#include <cstdint>

/// LCG with bitshift 32-bit random number generator.
/// taken from :
/// https://gist.github.com/itsmrpeck/0c55bc45c69632c49a480e9c51a2beaa
class quick_rng {
    uint64_t state;
    uint32_t value;

    void update() noexcept
    {
        state = (2862933555777941757 * state) + 3037000493;
        uint8_t shift = 29 - (state >> 61);
        value = uint32_t(state >> shift);
    }

public:
    quick_rng(uint64_t seed = 478119476) noexcept
    {
        state = seed;
        update();
    }

    // Gets the next random number (in 0 ... `UINT32_MAX`).
    uint32_t next() noexcept
    {
        update();
        return this->value;
    }
};

class invoker {
    using rng_type = std::mt19937;
    using dist_type = std::discrete_distribution<>;

    rng_type rng_;
    dist_type distribution_;

    std::vector<std::function<void()>> functions_;
    std::vector<double> weights_;

public:
    invoker(uint64_t seed) noexcept : rng_(uint32_t(seed)) {}

    void add(double weight, std::function<void()> fn)
    {
        functions_.push_back(std::move(fn));
        weights_.push_back(weight);
        distribution_ = dist_type(weights_.begin(), weights_.end());
    }

    int next() noexcept
    {
        size_t idx = distribution_(rng_); 
        functions_[idx]();
        return int(idx);
    }

    void run(size_t iterations) noexcept
    {
        for (size_t k = 0; k < iterations; k++)
            next();
    }
};

inline std::string random_ascii_string(size_t len, quick_rng& rng)
{
    std::string r;
    r.reserve(len + 1);
    for (size_t k = 0u; k < len; k++) {
        char ch = char(32 + rng.next() % 95);
        assert(ch >= 0);
        r.push_back(ch);
    }
    return r;
}

inline std::string random_ascii_string(
    size_t min_len, size_t max_len, quick_rng& rng)
{
    if (min_len > max_len)
        std::swap(min_len, max_len);
    size_t len = min_len + rng.next() % (max_len - min_len);
    return random_ascii_string(len, rng);
}
