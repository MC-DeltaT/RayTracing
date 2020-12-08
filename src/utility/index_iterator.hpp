#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>


class IndexIterator {
public:
	using value_type = std::size_t;
	using reference = std::size_t;
	using pointer = std::size_t const*;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::random_access_iterator_tag;

    IndexIterator() :
        _index{0}
    {}

	IndexIterator(std::size_t index) :
		_index{index}
	{}

	std::size_t index() const {
		return _index;
	}

	reference operator*() const {
		return _index;
	}

	reference operator[](difference_type index) const {
		assert(index > 0 || index <= _index);
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
	std::size_t _index;
};


inline bool operator==(IndexIterator const& lhs, IndexIterator const& rhs) {
    return lhs.index() == rhs.index();
}

inline bool operator!=(IndexIterator const& lhs, IndexIterator const& rhs) {
    return lhs.index() != rhs.index();
}

inline bool operator<(IndexIterator const& lhs, IndexIterator const& rhs) {
    return lhs.index() < rhs.index();
}

inline bool operator<=(IndexIterator const& lhs, IndexIterator const& rhs) {
    return lhs.index() <= rhs.index();
}

inline bool operator>(IndexIterator const& lhs, IndexIterator const& rhs) {
    return lhs.index() > rhs.index();
}

inline bool operator>=(IndexIterator const& lhs, IndexIterator const& rhs) {
    return lhs.index() >= rhs.index();
}

inline IndexIterator operator+(IndexIterator it, typename IndexIterator::difference_type n) {
    it += n;
    return it;
}

inline IndexIterator operator+(typename IndexIterator::difference_type n, IndexIterator it) {
    it += n;
    return it;
}

inline IndexIterator operator-(IndexIterator it, typename IndexIterator::difference_type n) {
    it -= n;
    return it;
}

inline typename IndexIterator::difference_type operator-(IndexIterator const& lhs,
        IndexIterator const& rhs) {
    return lhs.index() - rhs.index();
}
