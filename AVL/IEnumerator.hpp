#pragma once

template<typename T>
class IEnumerator {
public:
    virtual ~IEnumerator() = default;

    virtual bool moveNext() = 0;
    virtual const T& current() const = 0;
    virtual void reset() = 0;
};