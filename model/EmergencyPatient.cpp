#include "model/EmergencyPatient.h"

#include <cctype>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

EmergencyPatient::EmergencyPatient(std::string id, std::string name, int age, std::string contact)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {}

std::string EmergencyPatient::triage() const {
    std::string text = "IMMEDIATE ATTENTION - severity " + std::to_string(severity_) + "/5";
    if (!condition_.empty()) text += ": " + condition_;
    return text;
}

std::string EmergencyPatient::describe() const {
    return "[!] " + PatientRecord::describe() + ", severity " + std::to_string(severity_);
}

void EmergencyPatient::setExtraValues(const std::vector<std::string>& values) {
    condition_ = values.size() > 0 ? utils::trim(values[0]) : "";
    std::string sev = values.size() > 1 ? utils::trim(values[1]) : "3";
    if (sev.empty()) sev = "3";
    if (sev.size() != 1 || !std::isdigit(static_cast<unsigned char>(sev[0])) || sev[0] < '1' || sev[0] > '5')
        throw utils::ValidationException("Emergency severity must be a whole number from 1 to 5.");
    severity_ = sev[0] - '0';
}

void EmergencyPatient::scheduleAppointment(const Appointment& appointment) {
    Appointment tagged = appointment;
    if (tagged.reason().rfind("[URGENT]", 0) != 0) tagged.setReason("[URGENT] " + tagged.reason());
    PatientRecord::scheduleAppointment(tagged);
}

} // namespace scapms::model
