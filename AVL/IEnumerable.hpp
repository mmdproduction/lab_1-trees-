#pragma once

#include <memory>
#include <stdexcept>

#include "IEnumerator.hpp"

template<typename T>
class IEnumerable {
public:
    virtual ~IEnumerable() = default;

    virtual std::unique_ptr<IEnumerator<T>>
    getEnumerator() const = 0;

    struct Sentinel {};

    class Iterator {
    private:
        std::unique_ptr<IEnumerator<T>> enumerator_;
        bool hasCurrent_ = false;

    public:
        explicit Iterator(
            std::unique_ptr<IEnumerator<T>> enumerator
        ) : enumerator_(std::move(enumerator)) {
            if (!enumerator_) {
                throw std::invalid_argument("Null enumerator");
            }

            hasCurrent_ = enumerator_->moveNext();
        }

        const T& operator*() const {
            if (!hasCurrent_) {
                throw std::out_of_range("Iterator is at end");
            }

            return enumerator_->current();
        }

        Iterator& operator++() {
            if (hasCurrent_) {
                hasCurrent_ = enumerator_->moveNext();
            }

            return *this;
        }

        bool operator!=(Sentinel) const noexcept {
            return hasCurrent_;
        }

        bool operator==(Sentinel) const noexcept {
            return !hasCurrent_;
        }
    };

    Iterator begin() const {
        return Iterator(getEnumerator());
    }

    Sentinel end() const noexcept {
        return {};
    }
};