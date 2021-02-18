#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>


// Permutes the elements of an array using another array as indices.
template<typename T, typename IndexType>
class PermutationIterator {
public:
    using value_type = std::remove_cv_t<T>;
    using reference = T&;
    using pointer = T*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    PermutationIterator(T* elementIterator, IndexType const* indexIterator) :
        _elementIterator{elementIterator}, _indexIterator{indexIterator}
    {}

    T* elementIterator() const {
        return _elementIterator;
    }

    IndexType const* indexIterator() const {
        return _indexIterator;
    }

    reference operator*() const {
        return _elementIterator[*_indexIterator];
    }

    pointer operator->() const {
        return _elementIterator + *_indexIterator;
    }

    reference operator[](difference_type index) const {
        return _elementIterator[_indexIterator[index]];
    }

    PermutationIterator& operator++() {
        ++_indexIterator;
        return *this;
    }

    PermutationIterator operator++(int) {
        auto tmp = *this;
        ++_indexIterator;
        return tmp;
    }

    PermutationIterator& operator+=(difference_type n) {
        _indexIterator += n;
        return *this;
    }

    PermutationIterator& operator-=(difference_type n) {
        _indexIterator -= n;
        return *this;
    }

private:
    T* _elementIterator;
    IndexType const* _indexIterator;
};


template<typename T, typename IndexType>
bool operator==(PermutationIterator<T, IndexType> const& lhs, PermutationIterator<T, IndexType> const& rhs) {
    return lhs.indexIterator() == rhs.indexIterator();
}

template<typename T, typename IndexType>
bool operator!=(PermutationIterator<T, IndexType> const& lhs, PermutationIterator<T, IndexType> const& rhs) {
    return lhs.indexIterator() != rhs.indexIterator();
}

template<typename T, typename IndexType>
bool operator<(PermutationIterator<T, IndexType> const& lhs, PermutationIterator<T, IndexType> const& rhs) {
    return lhs.indexIterator() < rhs.indexIterator();
}

template<typename T, typename IndexType>
bool operator<=(PermutationIterator<T, IndexType> const& lhs, PermutationIterator<T, IndexType> const& rhs) {
    return lhs.indexIterator() <= rhs.indexIterator();
}

template<typename T, typename IndexType>
bool operator>(PermutationIterator<T, IndexType> const& lhs, PermutationIterator<T, IndexType> const& rhs) {
    return lhs.indexIterator() > rhs.indexIterator();
}

template<typename T, typename IndexType>
bool operator>=(PermutationIterator<T, IndexType> const& lhs, PermutationIterator<T, IndexType> const& rhs) {
    return lhs.indexIterator() >= rhs.indexIterator();
}

template<typename T, typename IndexType>
PermutationIterator<T, IndexType> operator+(PermutationIterator<T, IndexType> it,
        typename PermutationIterator<T, IndexType>::difference_type n) {
    it += n;
    return it;
}

template<typename T, typename IndexType>
PermutationIterator<T, IndexType> operator+(typename PermutationIterator<T, IndexType>::difference_type n,
        PermutationIterator<T, IndexType> it) {
    it += n;
    return it;
}

template<typename T, typename IndexType>
PermutationIterator<T, IndexType> operator-(PermutationIterator<T, IndexType> it,
        typename PermutationIterator<T, IndexType>::difference_type n) {
    it -= n;
    return it;
}

template<typename T, typename IndexType>
typename PermutationIterator<T, IndexType>::difference_type operator-(PermutationIterator<T, IndexType> const& lhs,
        PermutationIterator<T, IndexType> const& rhs) {
    return lhs.indexIterator() - rhs.indexIterator();
}
