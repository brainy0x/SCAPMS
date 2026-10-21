#include "model/FollowUpPatient.h"

#include "utils/DateTimeUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

FollowUpPatient::FollowUpPatient(std::string id, std::string name, int age, std::string contact)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {}

std::string FollowUpPatient::triage() const {
    std::string text = "Scheduled follow-up";
    if (!doctor_.empty()) text += " with " + doctor_;
    if (!lastVisit_.empty()) text += " (last visit " + lastVisit_ + ")";
    return text;
}

void FollowUpPatient::setExtraValues(const std::vector<std::string>& values) {
    std::string visit = values.size() > 0 ? utils::trim(values[0]) : "";
    if (!visit.empty() && !utils::isValidDate(visit))
        throw utils::ValidationException("Previous visit date must use the format yyyy-MM-dd.");
    lastVisit_ = visit;
    doctor_ = values.size() > 1 ? utils::trim(values[1]) : "";
}

void FollowUpPatient::scheduleAppointment(const Appointment& appointment) {
    Appointment tagged = appointment;
    if (tagged.reason().rfind("[Follow-up]", 0) != 0) tagged.setReason("[Follow-up] " + tagged.reason());
    PatientRecord::scheduleAppointment(tagged);
}

} // namespace scapms::model
