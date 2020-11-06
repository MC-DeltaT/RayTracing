#include "span.hpp"

#include <cassert>
#include <iterator>


template<typename T>
Span<T>::Span() :
    _data{nullptr}, _size{0}
{}

template<typename T>
Span<T>::Span(T* data, std::size_t size) :
    _data{data}, _size{size}
{}

template<typename T>
template<class Container>
Span<T>::Span(Container& container) :
    _data{std::data(container)}, _size{std::size(container)}
{}

template<typename T>
template<typename U>
Span<T>::Span(Span<U> other) :
    _data{other.data()}, _size{other.size()}
{}

template<typename T>
T* Span<T>::data() const {
    return _data;
}

template<typename T>
std::size_t Span<T>::size() const {
    return _size;
}

template<typename T>
T* Span<T>::begin() const {
    return _data;
}

template<typename T>
T* Span<T>::end() const {
    return _data + _size;
}

template<typename T>
T& Span<T>::operator[](std::size_t index) const {
    assert(index < _size);
    return _data[index];
}
