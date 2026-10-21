#pragma once
#include <stdexcept>
#include <string>

namespace scapms::utils {

// Base class for every error the clinic system raises on purpose.
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message) : std::runtime_error(message) {}
};

// Invalid patient information (bad name, age, contact, ...).
class ValidationException : public ClinicException {
public:
    explicit ValidationException(const std::string& message) : ClinicException(message) {}
};

// A patient with the same ID is already registered.
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(const std::string& id)
        : ClinicException("Patient ID '" + id + "' already exists.") {}
};

// Invalid appointment data (bad date/time, double booking, wrong status, ...).
class InvalidAppointmentException : public ClinicException {
public:
    explicit InvalidAppointmentException(const std::string& message) : ClinicException(message) {}
};

// A file could not be opened, read, written or parsed.
class FileException : public ClinicException {
public:
    explicit FileException(const std::string& message) : ClinicException(message) {}
};

// Invalid search input or an unsuitable search algorithm for the data state.
class SearchException : public ClinicException {
public:
    explicit SearchException(const std::string& message) : ClinicException(message) {}
};

// A requested patient / appointment does not exist.
class NotFoundException : public ClinicException {
public:
    explicit NotFoundException(const std::string& message) : ClinicException(message) {}
};

// An operation is not possible in the current state (empty queue, nothing to undo, ...).
class OperationException : public ClinicException {
public:
    explicit OperationException(const std::string& message) : ClinicException(message) {}
};

} // namespace scapms::utils
