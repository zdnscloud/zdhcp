#pragma once
#include <random>
#include <limits>

namespace kea{
namespace pinger{

template <typename T> 
class RandomGenerate{
    public:
        RandomGenerate() : distribution_(0, std::numeric_limits<T>::max()) {}

        RandomGenerate(const RandomGenerate& ) = delete;
        RandomGenerate& operator=(const RandomGenerate&) = delete;

        uint16_t getRandom() { return distribution_(random_); }

    private:
        std::random_device random_;
        std::uniform_int_distribution<> distribution_;
	
};

};
};
