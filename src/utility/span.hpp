#pragma once

#include "numeric.hpp"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>


template<typename T>
class Span {
public:
    using value_type = std::remove_cv_t<T>;
    using reference = T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = T*;

    Span() :
        _data{nullptr}, _size{0}
    {}

    Span(T* data, std::size_t size) :
        _data{data}, _size{size}
    {}

    template<class Container>
    explicit Span(Container& container) :
        _data{std::data(container)}, _size{std::size(container)}
    {}

    template<typename U>
    Span(Span<U> const& other) :
        _data{other.data()}, _size{other.size()}
    {}

    T* data() const {
        return _data;
    }

    iterator begin() const {
        return _data;
    }

    iterator end() const {
        return _data + _size;
    }

    size_type size() const {
        return _size;
    }

    reference operator[](size_type index) const {
        assert(index < _size);
        return _data[index];
    }

    template<typename IndexType, typename SizeType>
    Span<T> operator[](IndexRange<IndexType, SizeType> const& range) const {
        assert(range.end() <= _size);
        return {_data + range.begin, range.size};
    }

private:
    T* _data;
    std::size_t _size;
};

template<class Container>
Span(Container& container) -> Span<std::remove_pointer_t<decltype(std::data(container))>>;


template<class Container>
Span<typename Container::value_type const> readOnlySpan(Container const& container) {
    return Span{container};
}
