#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "types.hpp"

namespace usbsha256 {

template <std::size_t TMaxSize> class FastBuffer {
public:
    FastBuffer() = default;
    FastBuffer(BytesSpan bytes);
    FastBuffer(const Byte *const ptr, const std::size_t size);
    FastBuffer(const FastBuffer &) = default;
    FastBuffer(FastBuffer &&) = default;
    FastBuffer &operator=(const FastBuffer &) = default;
    FastBuffer &operator=(FastBuffer &&) = default;
    ~FastBuffer() = default;

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

template <std::size_t TMaxSize> FastBuffer<TMaxSize>::FastBuffer(BytesSpan bytes) { Assign(bytes); }

template <std::size_t TMaxSize>
FastBuffer<TMaxSize>::FastBuffer(const Byte *const ptr, const std::size_t size) {
    Assign({ptr, size});
}

template <std::size_t TMaxSize> void FastBuffer<TMaxSize>::Clear() { _size = 0; }

template <std::size_t TMaxSize> bool FastBuffer<TMaxSize>::Assign(BytesSpan bytes) {
    return Write(bytes, 0);
}

template <std::size_t TMaxSize> bool FastBuffer<TMaxSize>::Append(BytesSpan bytes) {
    return Write(bytes, _size);
}

// static
template <std::size_t TMaxSize> std::size_t FastBuffer<TMaxSize>::max_size() { return TMaxSize; }

template <std::size_t TMaxSize> std::size_t FastBuffer<TMaxSize>::DataSize() const { return _size; }

template <std::size_t TMaxSize> bool FastBuffer<TMaxSize>::Empty() const { return DataSize() == 0u; }

template <std::size_t TMaxSize> BytesSpan FastBuffer<TMaxSize>::Data() const {
    return {_data.data(), _size};
}

template <std::size_t TMaxSize>
bool FastBuffer<TMaxSize>::Write(BytesSpan bytes, const std::size_t pos) {
    if (pos + bytes.size() > TMaxSize) {
        return false;
    }
    std::copy(bytes.begin(), bytes.end(), _data.begin() + pos);
    _size = pos + bytes.size();
    return true;
}

} // namespace usbsha256
