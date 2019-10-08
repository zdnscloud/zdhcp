#pragma once

#include <kea/exceptions/exceptions.h>
#include <memory>

namespace kea {
namespace util {

template<typename ValueType>
class StagedValue {
public:

    StagedValue(): modified_(false) { }

    const ValueType& getValue() const {
        if (modified_) {
            return (*staging_);
        } else if (current_) {
            return (*current_);
        } else {
            kea_throw(Unexpected, "try to get uninitialized value");
        }
    }

    void setValue(std::unique_ptr<ValueType> new_value) {
        staging_ = std::move(new_value);
        modified_ = true;
    }

    void commit() {
        if (modified_) {
            current_ = std::move(staging_);
        }
        revert();
    }

    void reset() {
        revert();
        current_.reset();
    }

    void revert() {
        staging_.reset();
        modified_ = false;
    }

    StagedValue& operator=(std::unique_ptr<ValueType> value) {
        setValue(std::move(value));
        return (*this);
    }

    operator const ValueType&() const {
        return (getValue());
    }

private:
    std::unique_ptr<ValueType> staging_;
    std::unique_ptr<ValueType> current_;
    bool modified_;

};

}; 
};
