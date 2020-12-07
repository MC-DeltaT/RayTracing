#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <random>
#include <type_traits>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>


struct IndexRange {
    std::size_t begin;
    std::size_t size;

    std::size_t end() const {
        return begin + size;
    }
};


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

    template<class Container>
    Span(Container& container, IndexRange range) :
        Span{Span{container}[range]}
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

    Span<T> operator[](IndexRange range) const {
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
Span(Container& container, IndexRange const& range) -> Span<std::remove_pointer_t<decltype(std::data(container))>>;

template<class Container>
Span<typename Container::value_type const> readOnlySpan(Container const& container) {
    return Span{container};
}


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

    PermutationIterator operator++(int) const {
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


template<typename T>
class PermutedSpan {
public:
    using value_type = std::remove_cv_t<T>;
    using reference = T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = PermutationIterator<T>;

    PermutedSpan(Span<T> elements, Span<std::size_t const> indices) :
        _elements{elements}, _indices{indices}
    {}

    Span<T> elements() const {
        return _elements;
    }

    Span<std::size_t const> indices() const {
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

    PermutedSpan<T> operator[](IndexRange range) const {
        return {_elements, _indices[range]};
    }

private:
    Span<T> _elements;
    Span<std::size_t const> _indices;
};


class FastRNG {
public:
    FastRNG(std::uint64_t state) :
        _state{state}
    {}

    std::uint32_t value() {
        _state = (214013 * _state + 2531011); 
        return static_cast<std::uint32_t>(_state >> 16);
    }

    float unitFloat() {
        return static_cast<float>(value()) / 0xFFFFFFFF;
    }

    float angle() {
        return (glm::two_pi<float>() / 0xFFFFFFFF) * value();
    }

private:
    std::uint64_t _state;
};


template<typename T>
constexpr T square(T val) {
    return val * val;
}


template<unsigned Power, typename T>
constexpr T iPow(T val) {
    T result = 1;
    for (unsigned i = 0; i < Power; ++i) {
        result *= val;
    }
    return val;
}


inline bool isNormalised(float val) {
    return val >= 0.0f && val <= 1.0f;
}


inline bool isUnitVector(glm::vec3 const& vec) {
    return std::abs(glm::length(vec) - 1.0f) <= 1e-3f;
}


inline std::pair<glm::vec3, glm::vec3> orthonormalBasis(glm::vec3 const& vec) {
    assert(isUnitVector(vec));
    glm::vec3 const tmp{vec.z, vec.x, vec.y};       // TODO: fix for if all components equal
    auto const perpendicular1 = tmp - glm::dot(tmp, vec) * vec;
    auto const perpendicular2 = glm::cross(vec, perpendicular1);
    return {perpendicular1, perpendicular2};
}


inline static thread_local FastRNG randomEngine{std::random_device{}()};
