#include "model/PatientFactory.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

std::vector<std::string> PatientFactory::categories() { return {"General", "Emergency", "Follow-Up"}; }

namespace {
std::string normalise(const std::string& category) {
    std::string c = utils::toLower(utils::trim(category));
    if (c == "general") return "General";
    if (c == "emergency") return "Emergency";
    if (c == "follow-up" || c == "followup" || c == "follow up") return "Follow-Up";
    throw utils::ValidationException("Unknown patient category: '" + category + "'.");
}
} // namespace

std::vector<std::string> PatientFactory::extraLabelsFor(const std::string& category) {
    // Build a throw-away object so the labels always come from the subclass itself.
    return create(category, "TMP", "Temp", 0, "0000000", {})->extraLabels();
}

std::unique_ptr<PatientRecord> PatientFactory::create(const std::string& category, const std::string& id,
                                                      const std::string& name, int age,
                                                      const std::string& contact,
                                                      const std::vector<std::string>& extras) {
    std::string cat = normalise(category);
    std::unique_ptr<PatientRecord> patient;
    if (cat == "General") patient = std::make_unique<GeneralPatient>(id, name, age, contact);
    else if (cat == "Emergency") patient = std::make_unique<EmergencyPatient>(id, name, age, contact);
    else patient = std::make_unique<FollowUpPatient>(id, name, age, contact);
    patient->setExtraValues(extras);
    return patient;
}

} // namespace scapms::model
