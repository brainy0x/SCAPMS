#pragma once
#include "model/PatientRecord.h"

namespace scapms::model {

// A routine walk-in patient. Normal priority, standard queue.
class GeneralPatient : public PatientRecord {
public:
    GeneralPatient(std::string id, std::string name, int age, std::string contact);

    std::string category() const override { return "General"; }
    int priority() const override { return 3; }
    std::string triage() const override;
    std::vector<std::string> extraLabels() const override { return {"Insurance Provider", "Known Allergies"}; }
    std::vector<std::string> extraValues() const override { return {insurance_, allergies_}; }
    void setExtraValues(const std::vector<std::string>& values) override;
    std::unique_ptr<PatientRecord> clone() const override { return std::make_unique<GeneralPatient>(*this); }

private:
    std::string insurance_;
    std::string allergies_;
};

} // namespace scapms::model
