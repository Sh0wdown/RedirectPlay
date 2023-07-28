#pragma once

#include <type_traits>
#include <guiddef.h>
#include <string>

namespace std
{
    template<>
    struct hash<GUID>
    {
    public:
        using THashValue = size_t;

    private:
        template<typename T>
        static constexpr void GetSub(T value, THashValue& result, THashValue& subRes, size_t& subIndex) noexcept
        {
            size_t i = 0;
            while (i < sizeof(T))
            {
                size_t const shift = i * CHAR_BIT;
                THashValue const byte = (value & (0xFF << shift)) >> shift;

                subRes |= byte << (subIndex * CHAR_BIT);
                ++subIndex;

                if (subIndex >= sizeof(THashValue))
                {
                    result ^= subRes;
                    subRes = 0;
                    subIndex = 0;
                }
                ++i;
            }
        }

    public:
        constexpr hash() = default;
        constexpr THashValue operator()(const GUID& guid) const noexcept
        {
            THashValue result = 0, subRes = 0;

            size_t subIndex = 0;
            GetSub(guid.Data1, result, subRes, subIndex);
            GetSub(guid.Data2, result, subRes, subIndex);
            GetSub(guid.Data3, result, subRes, subIndex);
            for (size_t i = 0; i < 8; ++i)
                GetSub(guid.Data4[i], result, subRes, subIndex);

            return result ^ subRes;
        }
    };
}
