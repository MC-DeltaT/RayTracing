#pragma once

#include <cstddef>
#include <iterator>


template<typename T>
class Span {
public:
    Span();
    Span(T* data, std::size_t size);
    template<class Container>
    explicit Span(Container& container);
    template<typename U>
    Span(Span<U> other);

    T* data() const;
    std::size_t size() const;
    T* begin() const;
    T* end() const;

    T& operator[](std::size_t index) const;

private:
    T* _data;
    std::size_t _size;
};

template<class Container>
Span(Container& container) -> Span<std::remove_pointer_t<decltype(std::data(container))>>;


#include "span.tpp"
