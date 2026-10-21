#pragma once
#include <memory>
#include <string>
#include <vector>

#include "model/Appointable.h"
#include "model/Appointment.h"

namespace scapms::model {

// Abstract base class of every patient type. It owns the common data (ID, name, age,
// contact) and the patient's appointments (composition), and it implements the
// Appointable interface. The pure virtual functions force each subclass to supply
// behaviour that is specific to its patient type.
class PatientRecord : public Appointable {
public:
    PatientRecord(std::string id, std::string name, int age, std::string contact);
    ~PatientRecord() override = default;

    // ---- pure virtual: behaviour specific to each patient type ----
    virtual std::string category() const = 0;                   // "General", "Emergency", "Follow-Up"
    virtual int priority() const = 0;                           // 1 = most urgent
    virtual std::string triage() const = 0;                     // what staff should do with this patient
    virtual std::vector<std::string> extraLabels() const = 0;   // labels of the type-specific fields
    virtual std::vector<std::string> extraValues() const = 0;
    virtual void setExtraValues(const std::vector<std::string>& values) = 0;
    virtual std::unique_ptr<PatientRecord> clone() const = 0;   // deep copy (used by Undo)

    // ---- virtual with a sensible default ----
    virtual std::string describe() const;
    virtual bool allowsDoubleBooking() const { return false; }

    // ---- Appointable interface ----
    void scheduleAppointment(const Appointment& appointment) override;
    bool cancelAppointment(const std::string& appointmentId) override;
    std::size_t appointmentCount() const override { return appointments_.size(); }

    // ---- common data ----
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    int age() const { return age_; }
    const std::string& contact() const { return contact_; }
    void setName(const std::string& name) { name_ = validateName(name); }
    void setAge(int age) { age_ = validateAge(age); }
    void setContact(const std::string& contact) { contact_ = validateContact(contact); }

    // ---- appointments ----
    const std::vector<Appointment>& appointments() const { return appointments_; }
    const Appointment* findAppointment(const std::string& appointmentId) const;
    Appointment* findAppointment(const std::string& appointmentId);
    void restoreAppointment(const Appointment& appointment);           // no type-specific changes (file load / undo)
    bool removeAppointment(const std::string& appointmentId, Appointment& removed);
    int countWithStatus(AppointmentStatus status) const;
    int completedCount() const { return countWithStatus(AppointmentStatus::Completed); }
    int pendingCount() const { return countWithStatus(AppointmentStatus::Pending); }
    std::string nextAppointmentDateTime() const;                       // earliest pending, "" if none

    // ---- validation helpers (return the cleaned value or throw ValidationException) ----
    static std::string validateId(const std::string& id);
    static std::string validateName(const std::string& name);
    static int validateAge(int age);
    static std::string validateContact(const std::string& contact);
    static int parseAge(const std::string& text);

private:
    std::string id_;
    std::string name_;
    int age_ = 0;
    std::string contact_;
    std::vector<Appointment> appointments_;
};

} // namespace scapms::model
