#pragma once
#include <memory>
#include <string>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::model {

// Owns every registered patient (std::vector of base-class pointers => polymorphic
// storage) and, through them, every appointment. Patients are encapsulated: outside
// code adds/removes through this class so duplicate IDs can never get in.
class ClinicDatabase {
public:
    using PatientPtr = std::shared_ptr<PatientRecord>;

    void add(PatientPtr patient);                       // throws DuplicateIdException
    PatientPtr removeAt(std::size_t index);
    void insertAt(std::size_t index, PatientPtr patient);
    void replaceAt(std::size_t index, PatientPtr patient);
    void clear() { patients_.clear(); }

    int indexOf(const std::string& id) const;           // case-insensitive, -1 when missing
    bool contains(const std::string& id) const { return indexOf(id) >= 0; }
    PatientPtr find(const std::string& id) const;       // nullptr when missing
    PatientPtr findByAppointment(const std::string& appointmentId) const;

    std::size_t size() const { return patients_.size(); }
    bool empty() const { return patients_.empty(); }
    PatientPtr at(std::size_t index) const { return patients_.at(index); }

    std::vector<PatientPtr>& patients() { return patients_; }
    const std::vector<PatientPtr>& patients() const { return patients_; }

    std::vector<Appointment> allAppointments() const;

private:
    std::vector<PatientPtr> patients_;
};

} // namespace scapms::model
