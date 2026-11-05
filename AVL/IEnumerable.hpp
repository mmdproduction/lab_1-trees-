#pragma once

#include <memory>

#include "IEnumerator.hpp"

template<typename T>
class IEnumerable {
public:
    virtual ~IEnumerable() = default;

    virtual std::unique_ptr<IEnumerator<T>>
    getEnumerator() const = 0;
};