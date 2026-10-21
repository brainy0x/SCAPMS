#pragma once
#include <string>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

// One scheduled visit. Dates are stored as "yyyy-MM-dd" and times as "HH:mm", so
// plain string comparison also orders them chronologically.
class Appointment {
public:
    Appointment() = default;
    Appointment(std::string id, std::string patientId, std::string date, std::string time,
                std::string reason, AppointmentStatus status = AppointmentStatus::Pending);

    const std::string& id() const { return id_; }
    const std::string& patientId() const { return patientId_; }
    const std::string& date() const { return date_; }
    const std::string& time() const { return time_; }
    const std::string& reason() const { return reason_; }
    AppointmentStatus status() const { return status_; }
    std::string dateTime() const { return date_ + " " + time_; }

    void setReason(const std::string& reason) { reason_ = reason; }
    void setStatus(AppointmentStatus status) { status_ = status; }

    static std::string statusToString(AppointmentStatus status);
    static AppointmentStatus statusFromString(const std::string& text);

    // Throws InvalidAppointmentException when date/time/reason are malformed.
    static void validate(const std::string& date, const std::string& time, const std::string& reason);

private:
    std::string id_;
    std::string patientId_;
    std::string date_;
    std::string time_;
    std::string reason_;
    AppointmentStatus status_ = AppointmentStatus::Pending;
};

} // namespace scapms::model
