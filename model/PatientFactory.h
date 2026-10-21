#pragma once
#include <memory>
#include <string>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::model {

// Creates the right PatientRecord subclass from a category name. Used by the GUI
// (dynamic form) and by the file loader.
class PatientFactory {
public:
    static std::vector<std::string> categories();
    static std::vector<std::string> extraLabelsFor(const std::string& category);
    static std::unique_ptr<PatientRecord> create(const std::string& category, const std::string& id,
                                                 const std::string& name, int age,
                                                 const std::string& contact,
                                                 const std::vector<std::string>& extras);
};

} // namespace scapms::model
