#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <span>

namespace static_buffer {

using ElementType = std::uint8_t;
using SpanType = std::span<const ElementType>;

template <std::size_t TMaxSize>
class Buffer {
public:
    Buffer() = default;
    Buffer(SpanType span);
    Buffer(const ElementType* const ptr, const std::size_t size);
    Buffer(const Buffer&) = default;
    Buffer(Buffer&&) = default;
    Buffer& operator=(const Buffer&) = default;
    Buffer& operator=(Buffer&&) = default;
    ~Buffer() = default;

    void Clear();
    bool Assign(SpanType span);
    bool Append(SpanType span);

    static std::size_t max_size();
    std::size_t DataSize() const;
    bool Empty() const;
    SpanType Data() const;

private:
    bool Write(SpanType span, const std::size_t pos);

private:
    std::array<ElementType, TMaxSize> _data = {};
    std::size_t _size = {};
};

template <std::size_t TMaxSize>
Buffer<TMaxSize>::Buffer(SpanType span) {
    Assign(span);
}

template <std::size_t TMaxSize>
Buffer<TMaxSize>::Buffer(const ElementType* const ptr, const std::size_t size) {
    Assign({ptr, size});
}

template <std::size_t TMaxSize>
void Buffer<TMaxSize>::Clear() {
    _size = 0;
}

template <std::size_t TMaxSize>
bool Buffer<TMaxSize>::Assign(SpanType span) {
    return Write(span, 0);
}

template <std::size_t TMaxSize>
bool Buffer<TMaxSize>::Append(SpanType span) {
    return Write(span, _size);
}

// static
template <std::size_t TMaxSize>
std::size_t Buffer<TMaxSize>::max_size() {
    return TMaxSize;
}

template <std::size_t TMaxSize>
std::size_t Buffer<TMaxSize>::DataSize() const {
    return _size;
}

template <std::size_t TMaxSize>
bool Buffer<TMaxSize>::Empty() const {
    return DataSize() == 0u;
}

template <std::size_t TMaxSize>
SpanType Buffer<TMaxSize>::Data() const {
    return { _data.data(), _size };
}

template <std::size_t TMaxSize>
bool Buffer<TMaxSize>::Write(SpanType span, const std::size_t pos) {
    if (pos + span.size() > TMaxSize) {
        return false;
    }
    std::copy(span.begin(), span.end(), _data.begin() + pos);
    _size = pos + span.size();
    return true;
}

} // namespace static_buffer
