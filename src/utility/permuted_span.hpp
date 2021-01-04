#pragma once

#include "numeric.hpp"
#include "permutation_iterator.hpp"
#include "span.hpp"

#include <cstddef>
#include <type_traits>


template<typename T, typename IndexType = std::size_t>
class PermutedSpan {
public:
    using value_type = std::remove_cv_t<T>;
    using reference = T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = PermutationIterator<T, IndexType>;

    PermutedSpan(Span<T> elements, Span<IndexType const> indices) :
        _elements{elements}, _indices{indices}
    {}

    Span<T> elements() const {
        return _elements;
    }

    Span<IndexType const> indices() const {
        return _indices;
    }

    size_type size() const {
        return _indices.size();
    }

    iterator begin() const {
        return {_elements.begin(), _indices.begin()};
    }

    iterator end() const {
        return {_elements.begin(), _indices.end()};
    }

    reference operator[](size_type index) const {
        return _elements[_indices[index]];
    }

    template<typename U, typename V>
    PermutedSpan<T, IndexType> operator[](IndexRange<U, V> const& range) const {
        return {_elements, _indices[range]};
    }

private:
    Span<T> _elements;
    Span<IndexType const> _indices;
};
