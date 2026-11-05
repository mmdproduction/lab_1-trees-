#pragma once

#include <cstddef>

#include "IEnumerable.hpp"

template<typename T>
class ICollection : public IEnumerable<T> {
public:
    virtual ~ICollection() = default;

    virtual bool insert(const T& value) = 0;
    virtual bool erase(const T& value) = 0;
    virtual bool contains(const T& value) const = 0;

    virtual std::size_t size() const noexcept = 0;
    virtual bool empty() const noexcept = 0;

    virtual void clear() noexcept = 0;
};