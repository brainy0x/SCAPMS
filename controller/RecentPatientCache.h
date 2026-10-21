#pragma once
#include <cstddef>

#include "model/PatientRecord.h"

namespace scapms::controller {

// Small fixed-size cache of "Frequently Accessed Patient Records" built on a plain C++
// array. Slots are kept ordered by access count (ties: most recent first). When the
// array is full the least frequently used record (last slot) is replaced.
class RecentPatientCache {
public:
    static constexpr std::size_t Capacity = 10;

    RecentPatientCache() { clear(); }

    void touch(model::PatientRecord* patient);
    void remove(const model::PatientRecord* patient);
    void clear();

    std::size_t size() const { return count_; }
    model::PatientRecord* at(std::size_t slot) const { return slot < count_ ? recentPatients_[slot] : nullptr; }
    int accessCount(std::size_t slot) const { return slot < count_ ? accessCounts_[slot] : 0; }

private:
    model::PatientRecord* recentPatients_[Capacity];
    int accessCounts_[Capacity];
    std::size_t count_ = 0;
};

} // namespace scapms::controller
