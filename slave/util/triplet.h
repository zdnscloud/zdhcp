#pragma once

#include <kea/exceptions/exceptions.h>

namespace kea {
namespace util {

template <class T>
class Triplet {
public:

    Triplet<T>& operator=(T other) {
        min_ = other;
        default_ = other;
        max_ = other;
        unspecified_ = false;
        return (*this);
    }

    operator T() const {
        return (default_);
    }

    Triplet()
        : min_(0), default_(0), max_(0),
          unspecified_(true) {
    }

    Triplet(T value)
        : min_(value), default_(value), max_(value),
          unspecified_(false) {
    }

    Triplet(T min, T def, T max)
        : min_(min), default_(def), max_(max),
          unspecified_(false) {
        if ( (min_ > def) || (def > max_) ) {
            kea_throw(BadValue, "Invalid triplet values.");
        }
    }

    T getMin() const { return (min_);}

    T get() const { return (default_); }

    T get(T hint) const {
        if (hint <= min_) {
            return (min_);
        }

        if (hint >= max_) {
            return (max_);
        }

        return (hint);
    }

    T getMax() const { return (max_); }

    bool unspecified() const {
        return (unspecified_);
    }

private:
    T min_;
    T max_;
    T default_;
    bool unspecified_;
};

}; 
};
