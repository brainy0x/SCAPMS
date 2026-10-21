#pragma once
#include <string>
#include <vector>

#include "model/ClinicDatabase.h"
#include "model/QueueEntry.h"

namespace scapms::controller {

// Builds the plain-text reports shown in the Reports tab and exported to file.
class ReportGenerator {
public:
    static std::string mostFrequentlyAttended(const model::ClinicDatabase& db, std::size_t topN = 5);
    static std::string pendingAppointments(const model::ClinicDatabase& db);
    static std::string categoryDistribution(const model::ClinicDatabase& db);
    static std::string dailySummary(const model::ClinicDatabase& db, const std::string& date);
    static std::string waitingQueue(const model::ClinicDatabase& db, const std::vector<model::QueueEntry>& queue);
    static std::string all(const model::ClinicDatabase& db, const std::vector<model::QueueEntry>& queue,
                           const std::string& date);
};

} // namespace scapms::controller
