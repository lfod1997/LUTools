// Created: 2022-05-27

#ifndef _IMAGE_HPP_
#define _IMAGE_HPP_

#include "color.hpp"
#include "pathutils.hpp"

#include <stdexcept>
#include <string>
#include <utility>

#include <stb_image.h>
#include <stb_image_write.h>

namespace Lutools {

/// \brief Wrapper class of \c stb_image facilities
class Image {
protected:
    int _w = 0;
    int _h = 0;
    int _file_channels = 0;

    unsigned char* _data;

    Color* _begin;
    Color* _end;

public:
#pragma region Move-only

    Image(const Image&) = delete;

    Image& operator=(const Image&) = delete;

    Image(Image&& src) noexcept:
        _w(src._w),
        _h(src._h),
        _file_channels(src._file_channels),
        _data(src._data),
        _begin(src._begin),
        _end(src._end) {
        src._data = nullptr;
    }

    Image& operator=(Image&& src) noexcept {
        _w = src._w;
        _h = src._h;
        _file_channels = src._file_channels;
        _data = src._data;
        _begin = src._begin;
        _end = src._end;
        src._data = nullptr;
        return *this;
    }

#pragma endregion

    explicit Image(const char* path):
        _data(stbi_load(path, &_w, &_h, &_file_channels, 4)) {
        if (!_data || _w <= 0 || _h <= 0 || _file_channels <= 0) {
            throw std::runtime_error {
                std::string { "failed to load image file \"" } + path + "\": " + stbi_failure_reason()
            };
        }

        _begin = reinterpret_cast<Color*>(_data);
        _end = reinterpret_cast<Color*>(_data + static_cast<ptrdiff_t>(_w) * static_cast<ptrdiff_t>(_h) * 4);
    }

    explicit Image(const std::string& path):
        Image(path.c_str()) {}

    virtual ~Image() {
        if (_data) {
            stbi_image_free(_data);
            _data = nullptr;
        }
    }

    /// \brief Returns the width of the image
    int getWidth() const noexcept { return _w; }
    /// \brief Returns the height of the image
    int getHeight() const noexcept { return _h; }
    /// \brief Returns the bit depth of the image, i.e. 8 x the number of channels
    int getFileBitDepth() const noexcept { return _file_channels * 8; }
    /// \brief Returns the total number of pixels on the image
    std::size_t getTotalPixels() const noexcept { return _w * _h; } // NOLINT(*)

    /// \brief Pixel-wise iterator \c begin
    Color* begin() noexcept { return _begin; }
    /// \brief Pixel-wise const iterator \c begin
    const Color* begin() const noexcept { return _begin; }
    /// \brief Pixel-wise iterator \c end
    Color* end() noexcept { return _end; }
    /// \brief Pixel-wise const iterator \c end
    const Color* end() const noexcept { return _end; }

    /// \brief Returns the RGBA color of the pixel at the given position
    Color& at(ptrdiff_t x, ptrdiff_t y) {
        if (x >= _w) { x = _w - 1; }
        if (x < 0) { x = 0; }
        if (y >= _h) { y = _h - 1; }
        if (y < 0) { y = 0; }

        unsigned char* p = _data + (y * _w + x) * 4;
        return *reinterpret_cast<Color*>(p);
    }

    /// \brief Returns the RGBA color of the pixel at the given position
    const Color& at(ptrdiff_t x, ptrdiff_t y) const { return const_cast<Image*>(this)->at(x, y); }

    /// \brief Returns the RGBA color of the pixel at the given position
    /// \param xy A \c std::pair of x (horizontal pixel index) and y (vertical pixel index)
    template <typename IntTy>
    Color& at(const std::pair<IntTy, IntTy>& xy) {
        return at(static_cast<ptrdiff_t>(xy.first), static_cast<ptrdiff_t>(xy.second));
    }

    /// \brief Returns the RGBA color of the pixel at the given position
    /// \param xy A \c std::pair of x (horizontal pixel index) and y (vertical pixel index)
    template <typename IntTy>
    const Color& at(const std::pair<IntTy, IntTy>& xy) const {
        return at(static_cast<ptrdiff_t>(xy.first), static_cast<ptrdiff_t>(xy.second));
    }

    /// \brief Returns the RGBA color of the pixel at the given position
    Color& operator()(ptrdiff_t x, ptrdiff_t y) { return at(x, y); }

    /// \brief Returns the RGBA color of the pixel at the given position
    const Color& operator()(ptrdiff_t x, ptrdiff_t y) const { return at(x, y); }

    /// \brief Returns the RGBA color of the pixel at the given position
    /// \param xy A \c std::pair of x (horizontal pixel index) and y (vertical pixel index)
    template <typename IntTy>
    Color& operator()(const std::pair<IntTy, IntTy>& xy) {
        return at(static_cast<ptrdiff_t>(xy.first), static_cast<ptrdiff_t>(xy.second));
    }

    /// \brief Returns the RGBA color of the pixel at the given position
    /// \param xy A \c std::pair of x (horizontal pixel index) and y (vertical pixel index)
    template <typename IntTy>
    const Color& operator()(const std::pair<IntTy, IntTy>& xy) const {
        return at(static_cast<ptrdiff_t>(xy.first), static_cast<ptrdiff_t>(xy.second));
    }

    /// \brief Saves the image file
    /// \param path Path of the image file created / \b overwritten
    void save(const char* path) const {
        const std::string ext = Pathutils::getExtensionName(path);
        bool bad_flag = false;

        if (ext == "png") {
            bad_flag = !stbi_write_png(path, _w, _h, 4, _data, _w * 4);
        } else if (ext == "jpg" || ext == "jpeg") {
            bad_flag = !stbi_write_jpg(path, _w, _h, 4, _data, 90);
        } else if (ext == "tga") {
            bad_flag = !stbi_write_tga(path, _w, _h, 4, _data);
        } else if (ext == "bmp") {
            bad_flag = !stbi_write_bmp(path, _w, _h, 4, _data);
        } else {
            bad_flag = true;
        }

        if (bad_flag) {
            throw std::runtime_error {
                std::string { "failed to write to image file \"" } + path + "\""
            };
        }
    }

    /// \brief Saves the image file
    /// \param path Path of the image file created / \b overwritten
    void save(const std::string& path) const {
        save(path.c_str());
    }
};
}

#endif // _IMAGE_HPP_
