#pragma once
#include <string>
#include <vector>

#include "model/Appointment.h"
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

namespace scapms::controller {

// Business rules for appointments: validation, double-booking checks, status changes.
class AppointmentController {
public:
    AppointmentController(model::ClinicDatabase& database, utils::IDGenerator& ids)
        : database_(database), ids_(ids) {}

    model::Appointment schedule(const std::string& patientId, const std::string& date,
                                const std::string& time, const std::string& reason);
    void cancel(const std::string& appointmentId);
    void complete(const std::string& appointmentId);
    model::Appointment remove(const std::string& appointmentId);

    // Pending appointments starting within the next `minutes` minutes, earliest first.
    std::vector<model::Appointment> upcoming(int minutes) const;

private:
    model::ClinicDatabase& database_;
    utils::IDGenerator& ids_;
};

} // namespace scapms::controller
