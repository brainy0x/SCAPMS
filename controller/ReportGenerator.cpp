#include "controller/ReportGenerator.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

#include "controller/SortEngine.h"
#include "utils/DateTimeUtils.h"

namespace scapms::controller {

using model::AppointmentStatus;

namespace {
std::string heading(const std::string& title) {
    return title + "\n" + std::string(title.size(), '=') + "\n";
}
} // namespace

std::string ReportGenerator::mostFrequentlyAttended(const model::ClinicDatabase& db, std::size_t topN) {
    using Row = std::pair<model::ClinicDatabase::PatientPtr, int>;
    std::vector<Row> rows;
    for (const auto& p : db.patients())
        if (p->completedCount() > 0) rows.emplace_back(p, p->completedCount());

    // Merge sort (stable): most completed appointments first.
    SortEngine::sort(rows, SortAlgorithm::Merge, [](const Row& a, const Row& b) { return a.second > b.second; });

    std::ostringstream out;
    out << heading("Most Frequently Attended Patients");
    if (rows.empty()) {
        out << "No completed appointments yet.\n";
        return out.str();
    }
    for (std::size_t i = 0; i < rows.size() && i < topN; ++i)
        out << (i + 1) << ". " << std::left << std::setw(22) << rows[i].first->name() << std::setw(8)
            << rows[i].first->id() << std::setw(11) << rows[i].first->category() << rows[i].second
            << " completed\n";
    return out.str();
}

std::string ReportGenerator::pendingAppointments(const model::ClinicDatabase& db) {
    std::ostringstream out;
    out << heading("Patients with Pending Appointments");
    int patients = 0, total = 0;
    for (const auto& p : db.patients()) {
        if (p->pendingCount() == 0) continue;
        ++patients;
        out << p->name() << " (" << p->id() << ", " << p->category() << ")\n";
        for (const auto& a : p->appointments()) {
            if (a.status() != AppointmentStatus::Pending) continue;
            ++total;
            out << "    " << a.id() << "  " << a.dateTime() << "  " << a.reason() << "\n";
        }
    }
    if (patients == 0) out << "No pending appointments.\n";
    else out << "\n" << patients << " patient(s), " << total << " pending appointment(s).\n";
    return out.str();
}

std::string ReportGenerator::categoryDistribution(const model::ClinicDatabase& db) {
    int general = 0, emergency = 0, followUp = 0;
    for (const auto& p : db.patients()) {
        if (p->category() == "General") ++general;
        else if (p->category() == "Emergency") ++emergency;
        else ++followUp;
    }
    std::ostringstream out;
    out << heading("Patient Category Distribution");
    out << std::left << std::setw(20) << "General Patients" << general << "\n"
        << std::setw(20) << "Emergency Patients" << emergency << "\n"
        << std::setw(20) << "Follow-Up Patients" << followUp << "\n"
        << std::setw(20) << "Total" << db.size() << "\n";
    return out.str();
}

std::string ReportGenerator::dailySummary(const model::ClinicDatabase& db, const std::string& date) {
    int total = 0, completed = 0, pending = 0, cancelled = 0;
    for (const auto& a : db.allAppointments()) {
        if (a.date() != date) continue;
        ++total;
        if (a.status() == AppointmentStatus::Completed) ++completed;
        else if (a.status() == AppointmentStatus::Pending) ++pending;
        else ++cancelled;
    }
    std::ostringstream out;
    out << heading("Daily Appointment Summary - " + date);
    out << "Total Appointments: " << total << "\nCompleted: " << completed << "\nPending: " << pending
        << "\nCancelled: " << cancelled << "\n";
    return out.str();
}

std::string ReportGenerator::waitingQueue(const model::ClinicDatabase& db, const std::vector<model::QueueEntry>& queue) {
    std::ostringstream out;
    out << heading("Waiting Queue Status");
    if (queue.empty()) {
        out << "Nobody is waiting.\n";
        return out.str();
    }
    std::time_t now = std::time(nullptr);
    for (std::size_t i = 0; i < queue.size(); ++i) {
        auto p = db.find(queue[i].patientId);
        out << (i + 1) << ". " << (p ? p->name() : queue[i].patientId) << " ("
            << queue[i].patientId << ")  waiting " << utils::formatDuration(static_cast<long>(now - queue[i].joined))
            << "\n";
    }
    return out.str();
}

std::string ReportGenerator::all(const model::ClinicDatabase& db, const std::vector<model::QueueEntry>& queue,
                                 const std::string& date) {
    return mostFrequentlyAttended(db) + "\n" + pendingAppointments(db) + "\n" + categoryDistribution(db) + "\n" +
           dailySummary(db, date) + "\n" + waitingQueue(db, queue);
}

} // namespace scapms::controller
