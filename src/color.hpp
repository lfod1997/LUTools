// Created: 2022-05-27

#ifndef _COLOR_HPP_
#define _COLOR_HPP_

#include <type_traits>

struct Color
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    unsigned char operator[](unsigned char channel) const noexcept
    {
        switch (channel)
        {
        case 0:
            return r;
        case 1:
            return g;
        case 2:
            return b;
        case 3:
            return a;
        default:
            return 0;
        }
    }

    unsigned int hash() const noexcept
    {
        return *reinterpret_cast<const unsigned int*>(this);
    }

    unsigned int getHexRGB() const noexcept
    {
        return (r << 16) | (g << 8) | b;
    }

    unsigned int getHexRGBA() const noexcept
    {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    bool operator==(const Color& other) const noexcept
    {
        return !(hash() ^ other.hash());
    }
};

template <>
struct std::hash<Color>
{
    unsigned int operator()(const Color& color) const noexcept { return color.hash(); }
};

#endif // _COLOR_HPP_
