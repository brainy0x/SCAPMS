#pragma once
#include <ctime>
#include <string>

namespace scapms::model {

// One patient waiting for a consultation.
struct QueueEntry {
    std::string patientId;
    std::time_t joined = 0;
};

} // namespace scapms::model
