#include "model/GeneralPatient.h"

#include "utils/StringUtils.h"

namespace scapms::model {

GeneralPatient::GeneralPatient(std::string id, std::string name, int age, std::string contact)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {}

std::string GeneralPatient::triage() const {
    std::string text = "Routine consultation - standard queue";
    if (!allergies_.empty() && utils::toLower(allergies_) != "none")
        text += " (allergies: " + allergies_ + ")";
    return text;
}

void GeneralPatient::setExtraValues(const std::vector<std::string>& values) {
    insurance_ = values.size() > 0 ? utils::trim(values[0]) : "";
    allergies_ = values.size() > 1 ? utils::trim(values[1]) : "";
}

} // namespace scapms::model
