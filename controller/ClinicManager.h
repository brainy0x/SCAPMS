#pragma once
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <string>
#include <vector>

#include "controller/AdminOperation.h"
#include "controller/AppointmentController.h"
#include "controller/RecentPatientCache.h"
#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"
#include "model/ClinicDatabase.h"
#include "model/QueueEntry.h"
#include "utils/IDGenerator.h"

namespace scapms::controller {

// The application's business logic. The GUI only talks to this class; it contains no
// Qt code. It owns the data structures required by the brief:
//   std::vector (patients, inside ClinicDatabase)   std::queue (waiting queue)
//   std::stack  (undo history)                       fixed array (RecentPatientCache)
class ClinicManager {
public:
    using PatientPtr = std::shared_ptr<model::PatientRecord>;

    struct SearchResult {
        std::vector<PatientPtr> matches;
        std::string algorithmUsed;
        std::string note;
    };

    ClinicManager();

    // ---- patients ----
    std::string generatePatientId() const;
    void addPatient(PatientPtr patient);
    void updatePatient(const std::string& id, PatientPtr replacement);   // undoable
    void deletePatient(const std::string& id);                          // undoable
    PatientPtr accessPatient(const std::string& id);                    // counts as an access for the cache
    const model::ClinicDatabase& database() const { return database_; }

    // ---- appointments ----
    model::Appointment scheduleAppointment(const std::string& patientId, const std::string& date,
                                           const std::string& time, const std::string& reason);
    void cancelAppointment(const std::string& appointmentId);           // undoable
    void completeAppointment(const std::string& appointmentId);         // undoable
    void removeAppointment(const std::string& appointmentId);           // undoable
    std::vector<model::Appointment> upcomingAppointments(int minutes) const;

    // ---- waiting queue (FIFO) ----
    void enqueue(const std::string& patientId);
    PatientPtr callNext();
    std::vector<model::QueueEntry> queueSnapshot() const;
    std::size_t queueSize() const { return waiting_.size(); }
    std::vector<model::QueueEntry> longWaiting(int minutes) const;

    // ---- undo (stack) ----
    bool canUndo() const { return !undoStack_.empty(); }
    std::size_t undoDepth() const { return undoStack_.size(); }
    std::string undoDescription() const;
    std::string undoLast();

    // ---- sorting / searching ----
    SortStats sort(SortAlgorithm algorithm, SortField field);
    SearchResult search(SearchField field, SearchAlgorithm algorithm, const std::string& key);
    std::optional<SortField> currentOrder() const;

    // ---- polymorphism demo: work through the base class only ----
    static std::string processPatient(const model::PatientRecord& patient);
    static std::vector<std::string> processPatients(const std::vector<PatientPtr>& patients);

    // ---- persistence ----
    void save(const std::string& path) const;
    void load(const std::string& path);
    void clearAll();

    const RecentPatientCache& cache() const { return cache_; }
    const std::vector<std::string>& activityLog() const { return activityLog_; }
    int waitAlertMinutes() const { return waitAlertMinutes_; }
    void setWaitAlertMinutes(int minutes) { waitAlertMinutes_ = minutes < 1 ? 1 : minutes; }

private:
    void log(const std::string& message);
    void removeFromQueue(const std::string& patientId);
    bool isQueued(const std::string& patientId) const;
    void rebuildIds();
    void pushUndo(AdminOperation operation);
    void clearUndo();
    PatientPtr snapshotOf(const std::string& patientId) const;

    model::ClinicDatabase database_;
    utils::IDGenerator ids_;
    AppointmentController appointments_;
    std::queue<model::QueueEntry> waiting_;
    std::stack<AdminOperation> undoStack_;
    RecentPatientCache cache_;
    std::vector<std::string> activityLog_;
    int waitAlertMinutes_ = 5;
};

} // namespace scapms::controller
