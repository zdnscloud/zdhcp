#pragma once

namespace kea {
namespace util {

template<typename T>
inline bool equalValues(const T& ptr1, const T& ptr2) {
    return (ptr1 && ptr2 && (*ptr1 == *ptr2));
}

template<typename T>
inline bool nullOrEqualValues(const T& ptr1, const T& ptr2) {
    return ((!ptr1 && !ptr2) || equalValues(ptr1, ptr2));
}

}; 
};
