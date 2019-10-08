#pragma once

#include <kea/dns/name.h>
#include <kea/util/buffer.h>

namespace kea {
namespace dns {

class LabelSequence {
    friend std::string Name::toText(bool) const;

public:
    static const size_t MAX_SERIALIZED_LENGTH =
        Name::MAX_WIRE + Name::MAX_LABELS + 1;

    static const LabelSequence& WILDCARD();

    explicit LabelSequence(const Name& name):
        data_(&name.ndata_[0]),
        offsets_(&name.offsets_[0]),
        first_label_(0),
        last_label_(name.getLabelCount() - 1)
    {}

    explicit LabelSequence(const void* buf);

    LabelSequence(const LabelSequence& src, uint8_t buf[MAX_SERIALIZED_LENGTH]);

    LabelSequence(const LabelSequence& ls):
        data_(ls.data_),
        offsets_(ls.offsets_),
        first_label_(ls.first_label_),
        last_label_(ls.last_label_)
    {}

    const uint8_t* getData(size_t* len) const;

    size_t getDataLength() const;

    size_t getSerializedLength() const;

    void serialize(void* buf, size_t buf_len) const;

    bool equals(const LabelSequence& other, bool case_sensitive = false) const;

    bool operator==(const LabelSequence& other) const {
        return (equals(other));
    }

    NameComparisonResult compare(const LabelSequence& other,
                                 bool case_sensitive = false) const;

    void stripLeft(size_t i);

    void stripRight(size_t i);

    size_t getLabelCount() const {
        return (last_label_ - first_label_ + 1);
    }

    std::string toText() const;

    void extend(const LabelSequence& labels,
                uint8_t buf[MAX_SERIALIZED_LENGTH]);

    size_t getHash(bool case_sensitive) const;

    bool isAbsolute() const;

private:
    std::string toText(bool omit_final_dot) const;

private:
    const uint8_t* data_;       // wire-format name data
    const uint8_t* offsets_;    // an array of offsets in data_ for the labels
    size_t first_label_;        // index of offsets_ for the first label
    size_t last_label_;         // index of offsets_ for the last label.
                                // can be equal to first_label_, but must not
                                // be smaller (the class ensures that)
};


std::ostream&
operator<<(std::ostream& os, const LabelSequence& label_sequence);

inline const LabelSequence&
LabelSequence::WILDCARD() {
    static const uint8_t wildcard_buf[4] = { 0x01, 0x00, 0x01, '*' };
    static const LabelSequence wild_ls(wildcard_buf);
    return (wild_ls);
}

};
}; 
