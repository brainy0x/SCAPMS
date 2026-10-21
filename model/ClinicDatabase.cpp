#include "model/ClinicDatabase.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

void ClinicDatabase::add(PatientPtr patient) {
    if (!patient) throw utils::ValidationException("Cannot register an empty patient record.");
    if (contains(patient->id())) throw utils::DuplicateIdException(patient->id());
    patients_.push_back(std::move(patient));
}

ClinicDatabase::PatientPtr ClinicDatabase::removeAt(std::size_t index) {
    if (index >= patients_.size()) throw utils::NotFoundException("Patient index out of range.");
    PatientPtr removed = patients_[index];
    patients_.erase(patients_.begin() + static_cast<std::ptrdiff_t>(index));
    return removed;
}

void ClinicDatabase::insertAt(std::size_t index, PatientPtr patient) {
    if (!patient) throw utils::ValidationException("Cannot insert an empty patient record.");
    if (contains(patient->id())) throw utils::DuplicateIdException(patient->id());
    if (index > patients_.size()) index = patients_.size();
    patients_.insert(patients_.begin() + static_cast<std::ptrdiff_t>(index), std::move(patient));
}

void ClinicDatabase::replaceAt(std::size_t index, PatientPtr patient) {
    if (index >= patients_.size()) throw utils::NotFoundException("Patient index out of range.");
    if (!patient) throw utils::ValidationException("Cannot store an empty patient record.");
    patients_[index] = std::move(patient);
}

int ClinicDatabase::indexOf(const std::string& id) const {
    std::string key = utils::toLower(utils::trim(id));
    for (std::size_t i = 0; i < patients_.size(); ++i)
        if (utils::toLower(patients_[i]->id()) == key) return static_cast<int>(i);
    return -1;
}

ClinicDatabase::PatientPtr ClinicDatabase::find(const std::string& id) const {
    int i = indexOf(id);
    return i < 0 ? nullptr : patients_[static_cast<std::size_t>(i)];
}

ClinicDatabase::PatientPtr ClinicDatabase::findByAppointment(const std::string& appointmentId) const {
    for (const auto& p : patients_)
        if (p->findAppointment(appointmentId)) return p;
    return nullptr;
}

std::vector<Appointment> ClinicDatabase::allAppointments() const {
    std::vector<Appointment> all;
    for (const auto& p : patients_)
        for (const auto& a : p->appointments()) all.push_back(a);
    return all;
}

} // namespace scapms::model
