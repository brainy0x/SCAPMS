#pragma once
#include "model/PatientRecord.h"

namespace scapms::model {

// A returning patient with a scheduled review. Appointments are tagged [Follow-up].
class FollowUpPatient : public PatientRecord {
public:
    FollowUpPatient(std::string id, std::string name, int age, std::string contact);

    std::string category() const override { return "Follow-Up"; }
    int priority() const override { return 2; }
    std::string triage() const override;
    std::vector<std::string> extraLabels() const override { return {"Previous Visit (yyyy-MM-dd)", "Attending Doctor"}; }
    std::vector<std::string> extraValues() const override { return {lastVisit_, doctor_}; }
    void setExtraValues(const std::vector<std::string>& values) override;
    std::unique_ptr<PatientRecord> clone() const override { return std::make_unique<FollowUpPatient>(*this); }

    void scheduleAppointment(const Appointment& appointment) override;

private:
    std::string lastVisit_;
    std::string doctor_;
};

} // namespace scapms::model
