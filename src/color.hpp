// Created: 2022-05-27

#ifndef _COLOR_HPP_
#define _COLOR_HPP_

#include <type_traits>

namespace Lutools {

/// \brief POD class of a specific RGBA color
/// \example
/// To construct, simply use the uniform Initialization syntax:
///     \code Color { 69, 69, 118, 255 }; \endcode
struct Color {
    /// \brief 8-bit quantized value on the R channel
    unsigned char r;
    /// \brief 8-bit quantized value on the G channel
    unsigned char g;
    /// \brief 8-bit quantized value on the B channel
    unsigned char b;
    /// \brief 8-bit quantized value on the Alpha channel
    unsigned char a;

    /// \brief Returns the value on a specific channel
    unsigned char operator[](unsigned char channel) const noexcept {
        switch (channel) {
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

    /// \brief Fast hasher, can be used as the key for indexing
    /// \return Implementation defined \c unsigned
    /// \note DO NOT use this to get the human-readable hashcode, use \c Color::getHexRGBA() or \c Color::getHexRGB() instead
    unsigned int hash() const noexcept {
        return *reinterpret_cast<const unsigned int*>(this);
    }

    /// \brief Returns the RGB hashcode value of the color, like the one you'll get from e.g. Photoshop, but numeric
    unsigned int getHexRGB() const noexcept {
        return (r << 16) | (g << 8) | b;
    }

    /// \brief Returns the RGBA hashcode value of the color, like the one you'll get from e.g. Photoshop, but numeric
    unsigned int getHexRGBA() const noexcept {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    /// \brief Fast comparison based on \c Color::hash()
    bool operator==(const Color& other) const noexcept {
        return !(hash() ^ other.hash());
    }
};
}

template <>
struct std::hash<Lutools::Color> {
    unsigned int operator()(const Lutools::Color& color) const noexcept { return color.hash(); }
};

#endif // _COLOR_HPP_
