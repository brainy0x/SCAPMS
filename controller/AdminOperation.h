#pragma once
#include <memory>
#include <string>

#include "model/Appointment.h"
#include "model/PatientRecord.h"
#include "model/QueueEntry.h"

namespace scapms::controller {

// One reversible administrative operation, stored on the Undo stack.
struct AdminOperation {
    enum class Type { DeletePatient, ModifyPatient, RemoveAppointment };

    Type type = Type::ModifyPatient;
    std::string description;
    std::shared_ptr<model::PatientRecord> patient;   // Delete: the removed patient. Modify: copy of the old state.
    std::size_t index = 0;                           // position in the patient vector
    model::Appointment appointment;                  // RemoveAppointment: the removed appointment
    bool wasQueued = false;                          // DeletePatient: patient was in the waiting queue
    model::QueueEntry queueEntry;
};

} // namespace scapms::controller
