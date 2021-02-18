#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>


// Iterates ascending over numerical indices.
template<typename T = std::size_t>
class IndexIterator {
public:
	using value_type = T;
	using reference = T;
	using pointer = T const*;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::random_access_iterator_tag;

    IndexIterator() :
        _index{0}
    {}

	IndexIterator(T index) :
		_index{index}
	{}

	T index() const {
		return _index;
	}

	reference operator*() const {
		return _index;
	}

	reference operator[](difference_type index) const {
		assert(index >= 0 || static_cast<T>(-index) <= _index);
		return _index + index;
	}

	IndexIterator& operator++() {
        ++_index;
        return *this;
    }

    IndexIterator operator++(int) {
        auto tmp = *this;
        ++_index;
        return tmp;
    }

    IndexIterator& operator+=(difference_type n) {
        _index += n;
        return *this;
    }

    IndexIterator& operator-=(difference_type n) {
        _index -= n;
        return *this;
    }

private:
	T _index;
};


template<typename T>
inline bool operator==(IndexIterator<T> const& lhs, IndexIterator<T> const& rhs) {
    return lhs.index() == rhs.index();
}

template<typename T>
inline bool operator!=(IndexIterator<T> const& lhs, IndexIterator<T> const& rhs) {
    return lhs.index() != rhs.index();
}

template<typename T>
inline bool operator<(IndexIterator<T> const& lhs, IndexIterator<T> const& rhs) {
    return lhs.index() < rhs.index();
}

template<typename T>
inline bool operator<=(IndexIterator<T> const& lhs, IndexIterator<T> const& rhs) {
    return lhs.index() <= rhs.index();
}

template<typename T>
inline bool operator>(IndexIterator<T> const& lhs, IndexIterator<T> const& rhs) {
    return lhs.index() > rhs.index();
}

template<typename T>
inline bool operator>=(IndexIterator<T> const& lhs, IndexIterator<T> const& rhs) {
    return lhs.index() >= rhs.index();
}

template<typename T>
inline IndexIterator<T> operator+(IndexIterator<T> it, typename IndexIterator<T>::difference_type n) {
    it += n;
    return it;
}

template<typename T>
inline IndexIterator<T> operator+(typename IndexIterator<T>::difference_type n, IndexIterator<T> it) {
    it += n;
    return it;
}

template<typename T>
inline IndexIterator<T> operator-(IndexIterator<T> it, typename IndexIterator<T>::difference_type n) {
    it -= n;
    return it;
}

template<typename T>
inline typename IndexIterator<T>::difference_type operator-(IndexIterator<T> const& lhs,
        IndexIterator<T> const& rhs) {
    return lhs.index() - rhs.index();
}
