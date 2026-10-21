#include "controller/ClinicManager.h"

#include <ctime>

#include "utils/DateTimeUtils.h"
#include "utils/Exceptions.h"
#include "utils/FileHandler.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

using model::Appointment;
using model::QueueEntry;

ClinicManager::ClinicManager() : appointments_(database_, ids_) {}

// ------------------------------------------------------------------ patients

std::string ClinicManager::generatePatientId() const {
    // Starts from the next free number; skips any ID that is already taken.
    utils::IDGenerator probe = ids_;
    std::string id = probe.peekPatientId();
    while (database_.contains(id)) {
        probe.observe(id);
        id = probe.peekPatientId();
    }
    return id;
}

void ClinicManager::addPatient(PatientPtr patient) {
    database_.add(patient);                 // throws on duplicate ID
    ids_.observe(patient->id());
    log("Registered " + patient->describe());
}

ClinicManager::PatientPtr ClinicManager::snapshotOf(const std::string& patientId) const {
    auto patient = database_.find(patientId);
    if (!patient) throw utils::NotFoundException("No patient with ID '" + patientId + "' is registered.");
    return PatientPtr(patient->clone());
}

void ClinicManager::updatePatient(const std::string& id, PatientPtr replacement) {
    int index = database_.indexOf(id);
    if (index < 0) throw utils::NotFoundException("No patient with ID '" + id + "' is registered.");
    if (!replacement) throw utils::ValidationException("Cannot store an empty patient record.");
    if (utils::toLower(replacement->id()) != utils::toLower(database_.at(index)->id()))
        throw utils::ValidationException("A patient's ID cannot be changed.");

    PatientPtr old = database_.at(static_cast<std::size_t>(index));
    for (const auto& a : old->appointments()) replacement->restoreAppointment(a);   // keep the history

    AdminOperation op;
    op.type = AdminOperation::Type::ModifyPatient;
    op.description = "Modify patient " + old->name() + " (" + old->id() + ")";
    op.patient = old;                              // the old object stays untouched, so it is the snapshot
    op.index = static_cast<std::size_t>(index);

    cache_.remove(old.get());
    database_.replaceAt(static_cast<std::size_t>(index), replacement);
    pushUndo(op);
    log("Modified " + replacement->describe());
}

void ClinicManager::deletePatient(const std::string& id) {
    int index = database_.indexOf(id);
    if (index < 0) throw utils::NotFoundException("No patient with ID '" + id + "' is registered.");

    PatientPtr patient = database_.at(static_cast<std::size_t>(index));
    AdminOperation op;
    op.type = AdminOperation::Type::DeletePatient;
    op.description = "Delete patient " + patient->name() + " (" + patient->id() + ")";
    op.patient = patient;
    op.index = static_cast<std::size_t>(index);
    for (const auto& entry : queueSnapshot())
        if (utils::toLower(entry.patientId) == utils::toLower(patient->id())) {
            op.wasQueued = true;
            op.queueEntry = entry;
        }

    cache_.remove(patient.get());
    database_.removeAt(static_cast<std::size_t>(index));
    removeFromQueue(patient->id());
    pushUndo(op);
    log("Deleted " + patient->describe());
}

ClinicManager::PatientPtr ClinicManager::accessPatient(const std::string& id) {
    auto patient = database_.find(id);
    if (!patient) throw utils::NotFoundException("No patient with ID '" + id + "' is registered.");
    cache_.touch(patient.get());
    return patient;
}

// -------------------------------------------------------------- appointments

Appointment ClinicManager::scheduleAppointment(const std::string& patientId, const std::string& date,
                                               const std::string& time, const std::string& reason) {
    Appointment a = appointments_.schedule(patientId, date, time, reason);
    log("Scheduled appointment " + a.id() + " for " + patientId + " on " + a.dateTime());
    return a;
}

void ClinicManager::cancelAppointment(const std::string& appointmentId) {
    auto patient = database_.findByAppointment(appointmentId);
    if (!patient) throw utils::NotFoundException("Appointment '" + appointmentId + "' was not found.");

    AdminOperation op;
    op.type = AdminOperation::Type::ModifyPatient;
    op.description = "Cancel appointment " + appointmentId;
    op.patient = PatientPtr(patient->clone());
    op.index = static_cast<std::size_t>(database_.indexOf(patient->id()));

    appointments_.cancel(appointmentId);          // may throw; the undo entry is only pushed on success
    pushUndo(op);
    log("Cancelled appointment " + appointmentId);
}

void ClinicManager::completeAppointment(const std::string& appointmentId) {
    auto patient = database_.findByAppointment(appointmentId);
    if (!patient) throw utils::NotFoundException("Appointment '" + appointmentId + "' was not found.");

    AdminOperation op;
    op.type = AdminOperation::Type::ModifyPatient;
    op.description = "Complete appointment " + appointmentId;
    op.patient = PatientPtr(patient->clone());
    op.index = static_cast<std::size_t>(database_.indexOf(patient->id()));

    appointments_.complete(appointmentId);
    pushUndo(op);
    log("Completed appointment " + appointmentId);
}

void ClinicManager::removeAppointment(const std::string& appointmentId) {
    Appointment removed = appointments_.remove(appointmentId);
    AdminOperation op;
    op.type = AdminOperation::Type::RemoveAppointment;
    op.description = "Remove appointment " + appointmentId;
    op.appointment = removed;
    pushUndo(op);
    log("Removed appointment " + appointmentId);
}

std::vector<Appointment> ClinicManager::upcomingAppointments(int minutes) const {
    return appointments_.upcoming(minutes);
}

// --------------------------------------------------------------------- queue

bool ClinicManager::isQueued(const std::string& patientId) const {
    for (const auto& e : queueSnapshot())
        if (utils::toLower(e.patientId) == utils::toLower(patientId)) return true;
    return false;
}

void ClinicManager::enqueue(const std::string& patientId) {
    auto patient = database_.find(patientId);
    if (!patient) throw utils::NotFoundException("No patient with ID '" + patientId + "' is registered.");
    if (isQueued(patient->id())) throw utils::OperationException(patient->name() + " is already in the waiting queue.");
    QueueEntry entry;
    entry.patientId = patient->id();
    entry.joined = std::time(nullptr);
    waiting_.push(entry);
    log(patient->name() + " joined the waiting queue");
}

ClinicManager::PatientPtr ClinicManager::callNext() {
    if (waiting_.empty()) throw utils::OperationException("The waiting queue is empty.");
    QueueEntry next = waiting_.front();
    waiting_.pop();
    auto patient = database_.find(next.patientId);
    if (!patient) throw utils::NotFoundException("The next patient is no longer registered.");
    cache_.touch(patient.get());
    log("Called " + patient->name() + " for consultation");
    return patient;
}

std::vector<QueueEntry> ClinicManager::queueSnapshot() const {
    // std::queue cannot be iterated, so copy it and drain the copy.
    std::vector<QueueEntry> entries;
    std::queue<QueueEntry> copy = waiting_;
    while (!copy.empty()) {
        entries.push_back(copy.front());
        copy.pop();
    }
    return entries;
}

std::vector<QueueEntry> ClinicManager::longWaiting(int minutes) const {
    std::vector<QueueEntry> result;
    std::time_t now = std::time(nullptr);
    for (const auto& e : queueSnapshot())
        if (now - e.joined >= static_cast<std::time_t>(minutes) * 60) result.push_back(e);
    return result;
}

void ClinicManager::removeFromQueue(const std::string& patientId) {
    std::queue<QueueEntry> rebuilt;
    while (!waiting_.empty()) {
        if (utils::toLower(waiting_.front().patientId) != utils::toLower(patientId)) rebuilt.push(waiting_.front());
        waiting_.pop();
    }
    waiting_ = rebuilt;
}

// ---------------------------------------------------------------------- undo

void ClinicManager::pushUndo(AdminOperation operation) { undoStack_.push(std::move(operation)); }

void ClinicManager::clearUndo() {
    while (!undoStack_.empty()) undoStack_.pop();
}

std::string ClinicManager::undoDescription() const {
    return undoStack_.empty() ? std::string("Nothing to undo") : undoStack_.top().description;
}

std::string ClinicManager::undoLast() {
    if (undoStack_.empty()) throw utils::OperationException("There is no administrative operation to undo.");
    AdminOperation op = undoStack_.top();     // copy first; only pop once the reversal succeeded

    switch (op.type) {
    case AdminOperation::Type::DeletePatient:
        database_.insertAt(op.index, op.patient);
        ids_.observe(op.patient->id());
        if (op.wasQueued && !isQueued(op.patient->id())) waiting_.push(op.queueEntry);
        break;
    case AdminOperation::Type::ModifyPatient: {
        int index = database_.indexOf(op.patient->id());
        if (index < 0) throw utils::OperationException("The patient no longer exists, so this change cannot be undone.");
        cache_.remove(database_.at(static_cast<std::size_t>(index)).get());
        database_.replaceAt(static_cast<std::size_t>(index), op.patient);
        break;
    }
    case AdminOperation::Type::RemoveAppointment: {
        auto patient = database_.find(op.appointment.patientId());
        if (!patient) throw utils::OperationException("The patient no longer exists, so this change cannot be undone.");
        patient->restoreAppointment(op.appointment);
        break;
    }
    }

    undoStack_.pop();
    log("Undone: " + op.description);
    return op.description;
}

// ------------------------------------------------------------ sort & search

SortStats ClinicManager::sort(SortAlgorithm algorithm, SortField field) {
    SortStats stats = SortEngine::sort(database_.patients(), algorithm, SortEngine::comparatorFor(field));
    log(SortEngine::algorithmName(algorithm) + " by " + SortEngine::fieldName(field));
    return stats;
}

std::optional<SortField> ClinicManager::currentOrder() const {
    const SortField fields[] = {SortField::Name, SortField::Id, SortField::Age, SortField::AppointmentDate};
    if (database_.size() < 2) return std::nullopt;
    for (SortField f : fields)
        if (SortEngine::isSorted(database_.patients(), SortEngine::comparatorFor(f))) return f;
    return std::nullopt;
}

ClinicManager::SearchResult ClinicManager::search(SearchField field, SearchAlgorithm algorithm,
                                                  const std::string& rawKey) {
    std::string key = utils::trim(rawKey);
    if (key.empty()) throw utils::SearchException("Please enter a search keyword.");
    if (key.size() > 60) throw utils::SearchException("The search keyword is too long (maximum 60 characters).");

    const auto& list = database_.patients();
    std::vector<int> hits;
    SearchResult result;

    if (field == SearchField::Category) {
        hits = SearchEngine::linearSearchByCategory(list, key);
        result.algorithmUsed = "Linear Search";
        result.note = "Category is not a sort key, so every record is checked in turn.";
    } else {
        SortField sortField = field == SearchField::Id ? SortField::Id : SortField::Name;
        bool sorted = SortEngine::isSorted(list, SortEngine::comparatorFor(sortField));
        std::string fieldName = SortEngine::fieldName(sortField);

        SearchAlgorithm chosen = algorithm;
        if (chosen == SearchAlgorithm::Auto) {
            if (sorted && list.size() > 1) {
                chosen = SearchAlgorithm::Binary;
                result.note = "Auto: the records are sorted by " + fieldName + ", so Binary Search is used (prefix match).";
            } else if (field == SearchField::Id) {
                chosen = SearchAlgorithm::Linear;
                result.note = "Auto: the records are not sorted by ID, so Linear Search is used.";
            } else {
                chosen = SearchAlgorithm::Recursive;
                result.note = "Auto: the records are not sorted by name, so the Recursive Search is used.";
            }
        } else if (chosen == SearchAlgorithm::Binary && !sorted) {
            throw utils::SearchException("Binary Search needs the records sorted by " + fieldName +
                                         ". Sort them first on the Search & Sort tab, or choose Auto.");
        }

        switch (chosen) {
        case SearchAlgorithm::Linear:
            if (field == SearchField::Id) {
                int i = SearchEngine::linearSearchById(list, key);
                if (i >= 0) hits.push_back(i);
            } else {
                hits = SearchEngine::linearSearchByName(list, key);
            }
            result.algorithmUsed = "Linear Search";
            break;
        case SearchAlgorithm::Binary:
            hits = SearchEngine::binarySearch(list, field, key);
            result.algorithmUsed = "Binary Search";
            break;
        default:
            hits = SearchEngine::recursiveSearch(list, field, key);
            result.algorithmUsed = "Recursive Search";
            break;
        }
    }

    for (int i : hits) {
        PatientPtr p = list[static_cast<std::size_t>(i)];
        cache_.touch(p.get());
        result.matches.push_back(p);
    }
    log(result.algorithmUsed + " for '" + key + "' -> " + std::to_string(result.matches.size()) + " match(es)");
    return result;
}

// ------------------------------------------------------------- polymorphism

std::string ClinicManager::processPatient(const model::PatientRecord& patient) {
    // Only the base-class interface is used: category() and triage() resolve at run time
    // to GeneralPatient / EmergencyPatient / FollowUpPatient behaviour.
    return "[" + patient.category() + "] " + patient.name() + " -> " + patient.triage();
}

std::vector<std::string> ClinicManager::processPatients(const std::vector<PatientPtr>& patients) {
    std::vector<std::string> lines;
    for (const auto& p : patients) lines.push_back(processPatient(*p));
    return lines;
}

// -------------------------------------------------------------- persistence

void ClinicManager::rebuildIds() {
    ids_.reset();
    for (const auto& p : database_.patients()) {
        ids_.observe(p->id());
        for (const auto& a : p->appointments()) ids_.observe(a.id());
    }
}

void ClinicManager::save(const std::string& path) const {
    utils::FileHandler::save(path, database_, queueSnapshot());
}

void ClinicManager::load(const std::string& path) {
    // Load into temporaries first so a broken file never damages the data already in memory.
    model::ClinicDatabase loaded;
    std::vector<QueueEntry> loadedQueue;
    utils::FileHandler::load(path, loaded, loadedQueue);

    database_ = std::move(loaded);
    while (!waiting_.empty()) waiting_.pop();
    for (const auto& e : loadedQueue)
        if (database_.contains(e.patientId) && !isQueued(e.patientId)) waiting_.push(e);
    cache_.clear();
    clearUndo();
    rebuildIds();
    log("Loaded " + std::to_string(database_.size()) + " patient(s) from " + path);
}

void ClinicManager::clearAll() {
    database_.clear();
    while (!waiting_.empty()) waiting_.pop();
    cache_.clear();
    clearUndo();
    ids_.reset();
    log("Started a new, empty clinic");
}

void ClinicManager::log(const std::string& message) {
    activityLog_.push_back(utils::clockString(std::time(nullptr)) + "  " + message);
    if (activityLog_.size() > 200) activityLog_.erase(activityLog_.begin());
}

} // namespace scapms::controller
