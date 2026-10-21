#include "model/Appointment.h"
#include "utils/DateTimeUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

Appointment::Appointment(std::string id, std::string patientId, std::string date, std::string time,
                         std::string reason, AppointmentStatus status)
    : id_(std::move(id)), patientId_(std::move(patientId)), date_(std::move(date)),
      time_(std::move(time)), reason_(std::move(reason)), status_(status) {}

std::string Appointment::statusToString(AppointmentStatus status) {
    switch (status) {
    case AppointmentStatus::Pending: return "Pending";
    case AppointmentStatus::Completed: return "Completed";
    case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Pending";
}

AppointmentStatus Appointment::statusFromString(const std::string& text) {
    std::string t = utils::toLower(utils::trim(text));
    if (t == "pending") return AppointmentStatus::Pending;
    if (t == "completed") return AppointmentStatus::Completed;
    if (t == "cancelled" || t == "canceled") return AppointmentStatus::Cancelled;
    throw utils::InvalidAppointmentException("Unknown appointment status: '" + text + "'.");
}

void Appointment::validate(const std::string& date, const std::string& time, const std::string& reason) {
    if (!utils::isValidDate(date))
        throw utils::InvalidAppointmentException("Invalid appointment date. Use the format yyyy-MM-dd.");
    if (!utils::isValidTime(time))
        throw utils::InvalidAppointmentException("Invalid appointment time. Use the format HH:mm.");
    std::string r = utils::trim(reason);
    if (r.empty())
        throw utils::InvalidAppointmentException("Appointment reason must not be empty.");
    if (r.size() > 120)
        throw utils::InvalidAppointmentException("Appointment reason must be at most 120 characters.");
}

} // namespace scapms::model
