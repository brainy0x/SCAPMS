#include "model/PatientRecord.h"

#include <cctype>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

PatientRecord::PatientRecord(std::string id, std::string name, int age, std::string contact)
    : id_(validateId(id)), name_(validateName(name)), age_(validateAge(age)),
      contact_(validateContact(contact)) {}

std::string PatientRecord::describe() const {
    return category() + " patient " + name_ + " (" + id_ + "), age " + std::to_string(age_);
}

void PatientRecord::scheduleAppointment(const Appointment& appointment) {
    if (appointment.patientId() != id_)
        throw utils::InvalidAppointmentException("The appointment belongs to a different patient.");
    if (findAppointment(appointment.id()))
        throw utils::InvalidAppointmentException("Appointment ID '" + appointment.id() + "' already exists.");
    appointments_.push_back(appointment);
}

bool PatientRecord::cancelAppointment(const std::string& appointmentId) {
    Appointment* a = findAppointment(appointmentId);
    if (!a) return false;
    if (a->status() == AppointmentStatus::Completed)
        throw utils::InvalidAppointmentException("A completed appointment cannot be cancelled.");
    if (a->status() == AppointmentStatus::Cancelled)
        throw utils::InvalidAppointmentException("The appointment is already cancelled.");
    a->setStatus(AppointmentStatus::Cancelled);
    return true;
}

const Appointment* PatientRecord::findAppointment(const std::string& appointmentId) const {
    for (const auto& a : appointments_)
        if (a.id() == appointmentId) return &a;
    return nullptr;
}

Appointment* PatientRecord::findAppointment(const std::string& appointmentId) {
    for (auto& a : appointments_)
        if (a.id() == appointmentId) return &a;
    return nullptr;
}

void PatientRecord::restoreAppointment(const Appointment& appointment) {
    appointments_.push_back(appointment);
}

bool PatientRecord::removeAppointment(const std::string& appointmentId, Appointment& removed) {
    for (std::size_t i = 0; i < appointments_.size(); ++i) {
        if (appointments_[i].id() == appointmentId) {
            removed = appointments_[i];
            appointments_.erase(appointments_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

int PatientRecord::countWithStatus(AppointmentStatus status) const {
    int n = 0;
    for (const auto& a : appointments_)
        if (a.status() == status) ++n;
    return n;
}

std::string PatientRecord::nextAppointmentDateTime() const {
    std::string best;
    for (const auto& a : appointments_) {
        if (a.status() != AppointmentStatus::Pending) continue;
        std::string dt = a.dateTime();
        if (best.empty() || dt < best) best = dt;
    }
    return best;
}

std::string PatientRecord::validateId(const std::string& id) {
    std::string t = utils::trim(id);
    if (t.empty()) throw utils::ValidationException("Patient ID must not be empty.");
    if (t.size() > 12) throw utils::ValidationException("Patient ID must be at most 12 characters.");
    for (char c : t)
        if (std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == '"')
            throw utils::ValidationException("Patient ID must not contain spaces, commas or quotes.");
    return t;
}

std::string PatientRecord::validateName(const std::string& name) {
    std::string t = utils::trim(name);
    if (t.empty()) throw utils::ValidationException("Patient name must not be empty.");
    if (t.size() > 60) throw utils::ValidationException("Patient name must be at most 60 characters.");
    bool hasLetter = false;
    for (char c : t) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isalpha(u) || u >= 128) { hasLetter = true; continue; }
        if (c == ' ' || c == '-' || c == '\'' || c == '.') continue;
        throw utils::ValidationException("Patient name may only contain letters, spaces, '-', '.' and apostrophes.");
    }
    if (!hasLetter) throw utils::ValidationException("Patient name must contain letters.");
    return t;
}

int PatientRecord::validateAge(int age) {
    if (age < 0 || age > 120) throw utils::ValidationException("Patient age must be between 0 and 120.");
    return age;
}

std::string PatientRecord::validateContact(const std::string& contact) {
    std::string t = utils::trim(contact);
    int digits = 0;
    for (char c : t) {
        if (std::isdigit(static_cast<unsigned char>(c))) ++digits;
        else if (c != '+' && c != '-' && c != ' ' && c != '(' && c != ')')
            throw utils::ValidationException(
                "Contact must be a phone number (digits, spaces, +, -, parentheses).");
    }
    if (digits < 7 || digits > 15)
        throw utils::ValidationException("Contact phone number must contain 7 to 15 digits.");
    return t;
}

int PatientRecord::parseAge(const std::string& text) {
    std::string t = utils::trim(text);
    if (t.empty() || t.size() > 4)
        throw utils::ValidationException("Patient age must be a whole number between 0 and 120.");
    for (char c : t)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw utils::ValidationException("Patient age must be a whole number between 0 and 120.");
    return validateAge(std::stoi(t));
}

} // namespace scapms::model
