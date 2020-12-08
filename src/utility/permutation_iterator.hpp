#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>


template<typename T>
class PermutationIterator {
public:
    using value_type = std::remove_cv_t<T>;
    using reference = T&;
    using pointer = T*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    PermutationIterator(T* elementIterator, std::size_t const* indexIterator) :
        _elementIterator{elementIterator}, _indexIterator{indexIterator}
    {}

    T* elementIterator() const {
        return _elementIterator;
    }

    std::size_t const* indexIterator() const {
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
    std::size_t const* _indexIterator;
};

template<typename T>
bool operator==(PermutationIterator<T> const& lhs, PermutationIterator<T> const& rhs) {
    return lhs.indexIterator() == rhs.indexIterator();
}

template<typename T>
bool operator!=(PermutationIterator<T> const& lhs, PermutationIterator<T> const& rhs) {
    return lhs.indexIterator() != rhs.indexIterator();
}

template<typename T>
bool operator<(PermutationIterator<T> const& lhs, PermutationIterator<T> const& rhs) {
    return lhs.indexIterator() < rhs.indexIterator();
}

template<typename T>
bool operator<=(PermutationIterator<T> const& lhs, PermutationIterator<T> const& rhs) {
    return lhs.indexIterator() <= rhs.indexIterator();
}

template<typename T>
bool operator>(PermutationIterator<T> const& lhs, PermutationIterator<T> const& rhs) {
    return lhs.indexIterator() > rhs.indexIterator();
}

template<typename T>
bool operator>=(PermutationIterator<T> const& lhs, PermutationIterator<T> const& rhs) {
    return lhs.indexIterator() >= rhs.indexIterator();
}

template<typename T>
PermutationIterator<T> operator+(PermutationIterator<T> it, typename PermutationIterator<T>::difference_type n) {
    it += n;
    return it;
}

template<typename T>
PermutationIterator<T> operator+(typename PermutationIterator<T>::difference_type n, PermutationIterator<T> it) {
    it += n;
    return it;
}

template<typename T>
PermutationIterator<T> operator-(PermutationIterator<T> it, typename PermutationIterator<T>::difference_type n) {
    it -= n;
    return it;
}

template<typename T>
typename PermutationIterator<T>::difference_type operator-(PermutationIterator<T> const& lhs,
        PermutationIterator<T> const& rhs) {
    return lhs.indexIterator() - rhs.indexIterator();
}
