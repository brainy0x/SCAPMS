#pragma once
#include <cstddef>
#include <string>

#include "model/Appointment.h"

namespace scapms::model {

// Interface-like abstract class: anything that can be booked for appointments.
class Appointable {
public:
    virtual ~Appointable() = default;

    virtual void scheduleAppointment(const Appointment& appointment) = 0;
    // Returns false when the appointment ID is unknown; throws if it cannot be cancelled.
    virtual bool cancelAppointment(const std::string& appointmentId) = 0;
    virtual std::size_t appointmentCount() const = 0;
};

} // namespace scapms::model
