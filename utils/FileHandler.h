#pragma once
#include <string>
#include <vector>

#include "model/ClinicDatabase.h"
#include "model/QueueEntry.h"

namespace scapms::utils {

// CSV persistence. One file holds everything:
//   PATIENT,id,category,name,age,contact,extra1,extra2
//   APPT,appointmentId,patientId,date,time,reason,status
//   QUEUE,patientId,joinedEpochSeconds
// All problems are reported as FileException with a readable message.
class FileHandler {
public:
    static void save(const std::string& path, const model::ClinicDatabase& database,
                     const std::vector<model::QueueEntry>& queue);
    static void load(const std::string& path, model::ClinicDatabase& database,
                     std::vector<model::QueueEntry>& queue);
    static void writeText(const std::string& path, const std::string& text);
};

} // namespace scapms::utils
