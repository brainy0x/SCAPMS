#pragma once
#include "model/PatientRecord.h"

namespace scapms::model {

// A patient needing urgent care. Highest priority; may be booked into an already
// occupied slot and every appointment is tagged [URGENT].
class EmergencyPatient : public PatientRecord {
public:
    EmergencyPatient(std::string id, std::string name, int age, std::string contact);

    std::string category() const override { return "Emergency"; }
    int priority() const override { return 1; }
    std::string triage() const override;
    std::string describe() const override;
    bool allowsDoubleBooking() const override { return true; }
    std::vector<std::string> extraLabels() const override { return {"Emergency Condition", "Severity (1-5)"}; }
    std::vector<std::string> extraValues() const override { return {condition_, std::to_string(severity_)}; }
    void setExtraValues(const std::vector<std::string>& values) override;
    std::unique_ptr<PatientRecord> clone() const override { return std::make_unique<EmergencyPatient>(*this); }

    void scheduleAppointment(const Appointment& appointment) override;

    int severity() const { return severity_; }

private:
    std::string condition_;
    int severity_ = 3;
};

} // namespace scapms::model
