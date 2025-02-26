#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "types.hpp"

namespace usbsha256 {

template <std::size_t TMaxSize> class Buffer {
public:
    Buffer() = default;
    Buffer(BytesSpan bytes);
    Buffer(const Byte *const ptr, const std::size_t size);
    Buffer(const Buffer &) = default;
    Buffer(Buffer &&) = default;
    Buffer &operator=(const Buffer &) = default;
    Buffer &operator=(Buffer &&) = default;
    ~Buffer() = default;

    void Clear();
    bool Assign(BytesSpan bytes);
    bool Append(BytesSpan bytes);

    static std::size_t max_size();
    std::size_t DataSize() const;
    bool Empty() const;
    BytesSpan Data() const;

private:
    bool Write(BytesSpan bytes, const std::size_t pos);

private:
    std::array<Byte, TMaxSize> _data = {};
    std::size_t _size = {};
};

template <std::size_t TMaxSize> Buffer<TMaxSize>::Buffer(BytesSpan bytes) { Assign(bytes); }

template <std::size_t TMaxSize>
Buffer<TMaxSize>::Buffer(const Byte *const ptr, const std::size_t size) {
    Assign({ptr, size});
}

template <std::size_t TMaxSize> void Buffer<TMaxSize>::Clear() { _size = 0; }

template <std::size_t TMaxSize> bool Buffer<TMaxSize>::Assign(BytesSpan bytes) {
    return Write(bytes, 0);
}

template <std::size_t TMaxSize> bool Buffer<TMaxSize>::Append(BytesSpan bytes) {
    return Write(bytes, _size);
}

// static
template <std::size_t TMaxSize> std::size_t Buffer<TMaxSize>::max_size() { return TMaxSize; }

template <std::size_t TMaxSize> std::size_t Buffer<TMaxSize>::DataSize() const { return _size; }

template <std::size_t TMaxSize> bool Buffer<TMaxSize>::Empty() const { return DataSize() == 0u; }

template <std::size_t TMaxSize> BytesSpan Buffer<TMaxSize>::Data() const {
    return {_data.data(), _size};
}

template <std::size_t TMaxSize>
bool Buffer<TMaxSize>::Write(BytesSpan bytes, const std::size_t pos) {
    if (pos + bytes.size() > TMaxSize) {
        return false;
    }
    std::copy(bytes.begin(), bytes.end(), _data.begin() + pos);
    _size = pos + bytes.size();
    return true;
}

} // namespace usbsha256
