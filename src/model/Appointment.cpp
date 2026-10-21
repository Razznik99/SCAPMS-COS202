// =============================================================================
//  File   : src/model/Appointment.cpp
// =============================================================================
#include "Appointment.h"

#include <vector>

#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"

namespace scapms::model {

using scapms::utils::AppointmentException;
using scapms::utils::parseCsvLine;
using scapms::utils::trim;

Appointment::Appointment(std::string id,
                         std::string patientId,
                         std::string date,
                         std::string time,
                         std::string serviceCode,
                         std::string clinician,
                         double fee,
                         std::string notes,
                         AppointmentStatus status)
    : m_id(std::move(id)),
      m_patientId(std::move(patientId)),
      m_date(std::move(date)),
      m_time(std::move(time)),
      m_serviceCode(std::move(serviceCode)),
      m_clinician(std::move(clinician)),
      m_notes(std::move(notes)),
      m_fee(fee),
      m_status(status) {}

void Appointment::validate() const {
    if (trim(m_id).empty()) {
        throw AppointmentException("The appointment identifier must not be empty.");
    }
    if (trim(m_patientId).empty()) {
        throw AppointmentException("An appointment must be linked to a patient.");
    }
    if (!utils::isValidIsoDate(m_date)) {
        throw AppointmentException("Invalid appointment date '" + m_date +
                                   "'. Expected a real date in YYYY-MM-DD format.");
    }
    if (!utils::isValidIsoTime(m_time)) {
        throw AppointmentException("Invalid appointment time '" + m_time +
                                   "'. Expected a 24-hour time in HH:MM format.");
    }
    if (trim(m_serviceCode).empty()) {
        throw AppointmentException("An appointment must reference a medical service.");
    }
    if (trim(m_clinician).empty()) {
        throw AppointmentException("An appointment must have an assigned clinician.");
    }
    if (m_fee < 0.0) {
        throw AppointmentException("The appointment fee cannot be negative.");
    }
}

bool Appointment::isValid() const noexcept {
    return !trim(m_id).empty() && !trim(m_patientId).empty() &&
           utils::isValidIsoDate(m_date) && utils::isValidIsoTime(m_time) &&
           !trim(m_serviceCode).empty() && !trim(m_clinician).empty() &&
           m_fee >= 0.0;
}

std::string Appointment::statusToText(AppointmentStatus status) {
    switch (status) {
    case AppointmentStatus::Pending:
        return "Pending";
    case AppointmentStatus::Completed:
        return "Completed";
    case AppointmentStatus::Cancelled:
        return "Cancelled";
    }
    return "Pending";
}

std::string Appointment::statusText() const {
    return statusToText(m_status);
}

AppointmentStatus Appointment::parseStatus(const std::string& text) {
    const std::string value = utils::toLower(trim(text));
    if (value == "completed") {
        return AppointmentStatus::Completed;
    }
    if (value == "cancelled" || value == "canceled") {
        return AppointmentStatus::Cancelled;
    }
    return AppointmentStatus::Pending;
}

std::string Appointment::serialize() const {
    std::vector<std::string> fields;
    fields.push_back(utils::escapeCsv(m_id));
    fields.push_back(utils::escapeCsv(m_patientId));
    fields.push_back(utils::escapeCsv(m_date));
    fields.push_back(utils::escapeCsv(m_time));
    fields.push_back(utils::escapeCsv(m_serviceCode));
    fields.push_back(utils::escapeCsv(m_clinician));
    fields.push_back(utils::escapeCsv(statusText()));
    fields.push_back(utils::formatDouble(m_fee, 2));
    fields.push_back(utils::escapeCsv(m_notes));
    return utils::join(fields, ",");
}

Appointment Appointment::deserialize(const std::string& csvLine) {
    const std::vector<std::string> fields = parseCsvLine(csvLine);
    if (fields.size() < 7) {
        throw AppointmentException(
            "Malformed appointment record: expected at least 7 fields but found " +
            std::to_string(fields.size()) + ".");
    }

    Appointment appointment;
    appointment.m_id = trim(fields[0]);
    appointment.m_patientId = trim(fields[1]);
    appointment.m_date = trim(fields[2]);
    appointment.m_time = trim(fields[3]);
    appointment.m_serviceCode = trim(fields[4]);
    appointment.m_clinician = trim(fields[5]);
    appointment.m_status = parseStatus(fields[6]);
    appointment.m_fee = fields.size() > 7 && !trim(fields[7]).empty()
                            ? std::stod(trim(fields[7]))
                            : 0.0;
    appointment.m_notes = fields.size() > 8 ? trim(fields[8]) : "";

    appointment.validate();
    return appointment;
}

} // namespace scapms::model
