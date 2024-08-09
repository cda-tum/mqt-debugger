#pragma once

#include <cstddef>

template <typename T> class Span {
public:
  Span(T* ptr, size_t count) : pointer(ptr), spanSize(count) {}

  T& operator[](size_t index) const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return pointer[index];
  }

  T* data() const { return pointer; }

  [[nodiscard]] size_t size() const { return spanSize; }

private:
  T* pointer;
  size_t spanSize;
};
