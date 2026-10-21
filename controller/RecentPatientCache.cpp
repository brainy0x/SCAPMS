#include "controller/RecentPatientCache.h"

#include <utility>

namespace scapms::controller {

void RecentPatientCache::clear() {
    for (std::size_t i = 0; i < Capacity; ++i) {
        recentPatients_[i] = nullptr;
        accessCounts_[i] = 0;
    }
    count_ = 0;
}

void RecentPatientCache::touch(model::PatientRecord* patient) {
    if (!patient) return;

    std::size_t slot = count_;                       // count_ means "not found"
    for (std::size_t i = 0; i < count_; ++i)
        if (recentPatients_[i] == patient) { slot = i; break; }

    if (slot == count_) {                            // new record
        if (count_ < Capacity) ++count_;             // free slot available
        slot = count_ - 1;                           // otherwise overwrite the least-used (last) slot
        recentPatients_[slot] = patient;
        accessCounts_[slot] = 1;
    } else {
        ++accessCounts_[slot];
    }

    // Bubble towards the front while at least as frequently used as the neighbour.
    while (slot > 0 && accessCounts_[slot] >= accessCounts_[slot - 1]) {
        std::swap(recentPatients_[slot], recentPatients_[slot - 1]);
        std::swap(accessCounts_[slot], accessCounts_[slot - 1]);
        --slot;
    }
}

void RecentPatientCache::remove(const model::PatientRecord* patient) {
    for (std::size_t i = 0; i < count_; ++i) {
        if (recentPatients_[i] != patient) continue;
        for (std::size_t j = i; j + 1 < count_; ++j) {
            recentPatients_[j] = recentPatients_[j + 1];
            accessCounts_[j] = accessCounts_[j + 1];
        }
        --count_;
        recentPatients_[count_] = nullptr;
        accessCounts_[count_] = 0;
        return;
    }
}

} // namespace scapms::controller
