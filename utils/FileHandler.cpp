#include "utils/FileHandler.h"

#include <fstream>

#include "model/PatientFactory.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::utils {

void FileHandler::save(const std::string& path, const model::ClinicDatabase& database,
                       const std::vector<model::QueueEntry>& queue) {
    std::ofstream out(path);
    if (!out) throw FileException("Cannot open '" + path + "' for writing.");

    out << "# SCAPMS data file v1\n";
    for (const auto& p : database.patients()) {
        auto extras = p->extraValues();
        out << "PATIENT," << csvEscape(p->id()) << ',' << csvEscape(p->category()) << ',' << csvEscape(p->name())
            << ',' << p->age() << ',' << csvEscape(p->contact());
        for (const auto& e : extras) out << ',' << csvEscape(e);
        out << '\n';
        for (const auto& a : p->appointments())
            out << "APPT," << csvEscape(a.id()) << ',' << csvEscape(a.patientId()) << ',' << a.date() << ','
                << a.time() << ',' << csvEscape(a.reason()) << ','
                << model::Appointment::statusToString(a.status()) << '\n';
    }
    for (const auto& q : queue) out << "QUEUE," << csvEscape(q.patientId) << ',' << static_cast<long long>(q.joined) << '\n';

    out.flush();
    if (!out) throw FileException("Writing to '" + path + "' failed (disk full or file locked?).");
}

void FileHandler::load(const std::string& path, model::ClinicDatabase& database,
                       std::vector<model::QueueEntry>& queue) {
    std::ifstream in(path);
    if (!in) throw FileException("Cannot open '" + path + "'. Check that the file exists.");

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        const std::string where = "Line " + std::to_string(lineNo) + ": ";
        try {
            std::vector<std::string> f = csvSplit(trimmed);
            if (f[0] == "PATIENT") {
                if (f.size() < 6) throw FileException("PATIENT record needs at least 6 fields.");
                std::vector<std::string> extras(f.begin() + 6, f.end());
                int age = model::PatientRecord::parseAge(f[4]);
                database.add(model::PatientFactory::create(f[2], f[1], f[3], age, f[5], extras));
            } else if (f[0] == "APPT") {
                if (f.size() != 7) throw FileException("APPT record needs exactly 7 fields.");
                auto patient = database.find(f[2]);
                if (!patient) throw FileException("appointment refers to unknown patient '" + f[2] + "'.");
                model::Appointment::validate(f[3], f[4], f[5]);
                patient->restoreAppointment(model::Appointment(
                    f[1], patient->id(), f[3], f[4], f[5], model::Appointment::statusFromString(f[6])));
            } else if (f[0] == "QUEUE") {
                if (f.size() != 3) throw FileException("QUEUE record needs exactly 3 fields.");
                model::QueueEntry entry;
                entry.patientId = f[1];
                entry.joined = static_cast<std::time_t>(std::stoll(f[2]));
                queue.push_back(entry);
            } else {
                throw FileException("unrecognised record type '" + f[0] + "'.");
            }
        } catch (const FileException& e) {
            throw FileException(where + e.what());
        } catch (const ClinicException& e) {
            throw FileException(where + e.what());
        } catch (const std::exception&) {
            throw FileException(where + "the record is malformed.");
        }
    }
}

void FileHandler::writeText(const std::string& path, const std::string& text) {
    std::ofstream out(path);
    if (!out) throw FileException("Cannot open '" + path + "' for writing.");
    out << text;
    out.flush();
    if (!out) throw FileException("Writing to '" + path + "' failed.");
}

} // namespace scapms::utils
