#pragma once

#include <array>

#include <target.h>

#include "atom_samples.h"

using fw::Distance;
using fw::RangeSet;
using fw::Target;

const std::array<int16_t, 256> alaw2lpcm{
    -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,  -7552,  -7296,  -8064,  -7808,  -6528,  -6272,
    -7040,  -6784,  -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,  -3776,  -3648,  -4032,  -3904,
    -3264,  -3136,  -3520,  -3392,  -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944, -30208, -29184,
    -32256, -31232, -26112, -25088, -28160, -27136, -11008, -10496, -12032, -11520, -8960,  -8448,  -9984,  -9472,
    -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568, -344,   -328,   -376,   -360,   -280,   -264,
    -312,   -296,   -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,   -88,    -72,    -120,   -104,
    -24,    -8,     -56,    -40,    -216,   -200,   -248,   -232,   -152,   -136,   -184,   -168,   -1376,  -1312,
    -1504,  -1440,  -1120,  -1056,  -1248,  -1184,  -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
    -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,   -944,   -912,   -1008,  -976,   -816,   -784,
    -880,   -848,   5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,   7552,   7296,   8064,   7808,
    6528,   6272,   7040,   6784,   2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,   3776,   3648,
    4032,   3904,   3264,   3136,   3520,   3392,   22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
    30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,  11008,  10496,  12032,  11520,  8960,   8448,
    9984,   9472,   15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,  344,    328,    376,    360,
    280,    264,    312,    296,    472,    456,    504,    488,    408,    392,    440,    424,    88,     72,
    120,    104,    24,     8,      56,     40,     216,    200,    248,    232,    152,    136,    184,    168,
    1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,   1888,   1824,   2016,   1952,   1632,   1568,
    1760,   1696,   688,    656,    752,    720,    560,    528,    624,    592,    944,    912,    1008,   976,
    816,    784,    880,    848,
};

class MyTarget : public Target<Value_t>
{
   public:
    ~MyTarget() override = default;

    MyTarget()
    {
        m_values.clear();
        for (std::size_t i = 0; i < VALUES_COUNT; ++i) {
            // NOLINTNEXTLINE(readability-magic-numbers)
            const auto a = (static_cast<uint8_t>(i - 128)) ^ 0x55;
            auto l = static_cast<Value_t>(alaw2lpcm[a]);
            m_values.push_back(l);
        }
    }

    [[nodiscard]] Distance Compare(const FuncValues_t& values) const override
    {
        Distance dist{};
        for (std::size_t i = VALUE_FIRST; i <= VALUE_LAST; ++i) {
            if (values[i] != m_values[i]) {
                ++dist;
            }
        }
        return dist;
    }

    [[nodiscard]] RangeSet<std::size_t> MatchPositions(const FuncValues_t& values) const override
    {
        RangeSet<std::size_t> rset;
        for (std::size_t i = VALUE_FIRST; i <= VALUE_LAST; ++i) {
            if (values[i] == m_values[i]) {
                rset.Add(i);
            }
        }
        return rset;
    }

    [[nodiscard]] FuncValues_t Values() const override { return m_values; }

    [[nodiscard]] std::string StrFull() const
    {
        std::string res = "TARGET ";
        for (const auto& v : m_values) {
            res += std::format("; {}", v);
        }
        return res;
    }

   private:
    FuncValues_t m_values;
};