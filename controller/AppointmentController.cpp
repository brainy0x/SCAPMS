#include "controller/AppointmentController.h"

#include <ctime>

#include "controller/SortEngine.h"
#include "utils/DateTimeUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

using model::Appointment;
using model::AppointmentStatus;

Appointment AppointmentController::schedule(const std::string& patientId, const std::string& date,
                                            const std::string& time, const std::string& reason) {
    auto patient = database_.find(patientId);
    if (!patient) throw utils::NotFoundException("No patient with ID '" + patientId + "' is registered.");

    Appointment::validate(date, time, reason);
    std::time_t when = utils::parseDateTime(date, time);
    if (when < std::time(nullptr))
        throw utils::InvalidAppointmentException("An appointment cannot be scheduled in the past.");

    // Double-booking rule: one pending appointment per slot, unless the patient type
    // (decided polymorphically) may be fitted in, e.g. emergencies.
    if (!patient->allowsDoubleBooking()) {
        for (const auto& other : database_.patients())
            for (const auto& a : other->appointments())
                if (a.status() == AppointmentStatus::Pending && a.date() == date && a.time() == time)
                    throw utils::InvalidAppointmentException(
                        "The time slot " + date + " " + time + " is already booked.");
    }

    std::string id = ids_.peekAppointmentId();
    ids_.observe(id);
    patient->scheduleAppointment(Appointment(id, patient->id(), date, time, utils::trim(reason)));
    return *patient->findAppointment(id);   // the stored copy (subclasses may tag the reason)
}

void AppointmentController::cancel(const std::string& appointmentId) {
    auto patient = database_.findByAppointment(appointmentId);
    if (!patient) throw utils::NotFoundException("Appointment '" + appointmentId + "' was not found.");
    patient->cancelAppointment(appointmentId);
}

void AppointmentController::complete(const std::string& appointmentId) {
    auto patient = database_.findByAppointment(appointmentId);
    if (!patient) throw utils::NotFoundException("Appointment '" + appointmentId + "' was not found.");
    Appointment* a = patient->findAppointment(appointmentId);
    if (a->status() != AppointmentStatus::Pending)
        throw utils::InvalidAppointmentException("Only pending appointments can be marked as completed.");
    a->setStatus(AppointmentStatus::Completed);
}

Appointment AppointmentController::remove(const std::string& appointmentId) {
    auto patient = database_.findByAppointment(appointmentId);
    if (!patient) throw utils::NotFoundException("Appointment '" + appointmentId + "' was not found.");
    Appointment removed;
    patient->removeAppointment(appointmentId, removed);
    return removed;
}

std::vector<Appointment> AppointmentController::upcoming(int minutes) const {
    std::vector<Appointment> result;
    std::time_t now = std::time(nullptr);
    for (const auto& p : database_.patients()) {
        for (const auto& a : p->appointments()) {
            if (a.status() != AppointmentStatus::Pending) continue;
            std::time_t when = utils::parseDateTime(a.date(), a.time());
            if (when >= now && when - now <= static_cast<std::time_t>(minutes) * 60) result.push_back(a);
        }
    }
    SortEngine::sort(result, SortAlgorithm::Insertion,
                     [](const Appointment& x, const Appointment& y) { return x.dateTime() < y.dateTime(); });
    return result;
}

} // namespace scapms::controller
