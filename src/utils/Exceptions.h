// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/utils/Exceptions.h
//  Layer  : Utilities
//  Purpose: Custom exception hierarchy used across every layer of the system.
//           Every exception carries a human-readable message and a short code
//           so the GUI can present meaningful, non-fatal error dialogs.
// =============================================================================
#ifndef SCAPMS_UTILS_EXCEPTIONS_H
#define SCAPMS_UTILS_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace scapms::utils {

/// Base class for every recoverable SCAPMS error.
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message,
                             std::string code = "CLINIC_ERROR")
        : std::runtime_error(message), m_code(std::move(code)) {}

    [[nodiscard]] const std::string& code() const noexcept { return m_code; }

private:
    std::string m_code;
};

/// Invalid user input (empty name, bad age, malformed contact, empty query...).
class ValidationException : public ClinicException {
public:
    explicit ValidationException(const std::string& message,
                                 std::string field = "")
        : ClinicException(message, "VALIDATION_ERROR"), m_field(std::move(field)) {}

    [[nodiscard]] const std::string& field() const noexcept { return m_field; }

private:
    std::string m_field;
};

/// A patient / appointment identifier that already exists.
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(const std::string& id)
        : ClinicException("The identifier '" + id + "' already exists. "
                          "Please use a unique identifier.",
                          "DUPLICATE_ID"),
          m_id(id) {}

    [[nodiscard]] const std::string& id() const noexcept { return m_id; }

private:
    std::string m_id;
};

/// A record that was requested but is not present in the database.
class NotFoundException : public ClinicException {
public:
    explicit NotFoundException(const std::string& what, const std::string& id)
        : ClinicException(what + " '" + id + "' was not found.", "NOT_FOUND"),
          m_id(id) {}

    [[nodiscard]] const std::string& id() const noexcept { return m_id; }

private:
    std::string m_id;
};

/// Scheduling problems: past dates, clashes, non-appointable patient types.
class AppointmentException : public ClinicException {
public:
    explicit AppointmentException(const std::string& message)
        : ClinicException(message, "APPOINTMENT_ERROR") {}
};

/// Persistence problems: missing, unreadable or malformed data files.
class FileException : public ClinicException {
public:
    FileException(const std::string& message, const std::string& path)
        : ClinicException(message + " (file: " + path + ")", "FILE_ERROR"),
          m_path(path) {}

    [[nodiscard]] const std::string& path() const noexcept { return m_path; }

private:
    std::string m_path;
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_EXCEPTIONS_H
