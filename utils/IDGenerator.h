#pragma once
#include <cstdio>
#include <string>

namespace scapms::utils {

// Produces readable IDs ("P001", "A0001"). It never hands out an ID twice because
// every ID that enters the system (typed, loaded from file) is passed to observe().
class IDGenerator {
public:
    void reset() { patientCounter_ = 0; appointmentCounter_ = 0; }

    void observe(const std::string& id) {
        if (id.empty()) return;
        int value = numericPart(id);
        if (value < 0) return;
        if (id[0] == 'P' || id[0] == 'p') { if (value > patientCounter_) patientCounter_ = value; }
        else if (id[0] == 'A' || id[0] == 'a') { if (value > appointmentCounter_) appointmentCounter_ = value; }
    }

    // peek* does not consume the number, so a GUI can show the "next" ID safely.
    std::string peekPatientId() const { return format('P', patientCounter_ + 1, 3); }
    std::string peekAppointmentId() const { return format('A', appointmentCounter_ + 1, 4); }

private:
    static int numericPart(const std::string& id) {
        if (id.size() < 2 || id.size() > 9) return -1;
        int value = 0;
        for (std::size_t i = 1; i < id.size(); ++i) {
            if (id[i] < '0' || id[i] > '9') return -1;
            value = value * 10 + (id[i] - '0');
        }
        return value;
    }
    static std::string format(char prefix, int number, int width) {
        char buffer[24];
        std::snprintf(buffer, sizeof buffer, "%c%0*d", prefix, width, number);
        return buffer;
    }

    int patientCounter_ = 0;
    int appointmentCounter_ = 0;
};

} // namespace scapms::utils
