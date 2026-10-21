// =============================================================================
//  File   : src/controller/ReportGenerator.cpp
//  Layer  : Controller
//  Purpose: Aggregates the database into report rows. Ordering inside the
//           reports uses hand-written insertion sort (no std::sort).
// =============================================================================
#include "ReportGenerator.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "../model/Appointment.h"
#include "../model/PatientRecord.h"
#include "../utils/DateUtils.h"
#include "../utils/StringUtils.h"

namespace scapms::controller {

namespace {

/// Classic hand-written insertion sort on a vector of report rows using a
/// caller-supplied "less" predicate.
template <typename T, typename Less>
void insertionSortRows(std::vector<T>& items, Less less) {
    for (std::size_t i = 1; i < items.size(); ++i) {
        T current = items[i];
        std::size_t j = i;
        while (j > 0 && less(current, items[j - 1])) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = current;
    }
}

std::string pad(const std::string& text, std::size_t width) {
    if (text.size() >= width) {
        return text;
    }
    return text + std::string(width - text.size(), ' ');
}

std::string percent(std::size_t part, std::size_t total) {
    if (total == 0) {
        return "0.0%";
    }
    const double value = 100.0 * static_cast<double>(part) / static_cast<double>(total);
    return scapms::utils::formatDouble(value, 1) + "%";
}

} // namespace

ReportGenerator::ReportGenerator(const model::ClinicDatabase& database)
    : m_database(database) {}

std::string ReportGenerator::name(ReportType type) {
    switch (type) {
    case ReportType::FrequentAttendees:
        return "Most Frequently Attended Patients";
    case ReportType::PendingAppointments:
        return "Patients with Pending Appointments";
    case ReportType::CategoryDistribution:
        return "Patient Category Distribution";
    case ReportType::DailySummary:
        return "Daily Appointment Summary";
    case ReportType::RevenueByService:
        return "Revenue by Medical Service";
    }
    return "Report";
}

std::vector<ReportType> ReportGenerator::all() {
    return {ReportType::FrequentAttendees, ReportType::PendingAppointments,
            ReportType::CategoryDistribution, ReportType::DailySummary,
            ReportType::RevenueByService};
}

ReportResult ReportGenerator::generate(ReportType type, const std::string& date) const {
    switch (type) {
    case ReportType::FrequentAttendees:
        return frequentAttendees();
    case ReportType::PendingAppointments:
        return pendingAppointments();
    case ReportType::CategoryDistribution:
        return categoryDistribution();
    case ReportType::DailySummary:
        return dailySummary(date.empty() ? scapms::utils::todayIso() : date);
    case ReportType::RevenueByService:
        return revenueByService();
    }
    return {};
}

ReportResult ReportGenerator::frequentAttendees() const {
    struct Tally {
        std::string id;
        std::string name;
        std::string category;
        int completed = 0;
        std::string lastVisit;
    };

    std::vector<Tally> tallies;
    for (const auto& appointment : m_database.appointments()) {
        if (!appointment.isCompleted()) {
            continue;
        }
        std::size_t found = tallies.size();
        for (std::size_t i = 0; i < tallies.size(); ++i) {
            if (tallies[i].id == appointment.patientId()) {
                found = i;
                break;
            }
        }
        if (found == tallies.size()) {
            Tally tally;
            tally.id = appointment.patientId();
            const auto* patient = m_database.findPatientById(appointment.patientId());
            tally.name = patient ? patient->name() : "(deleted patient)";
            tally.category = patient ? patient->category() : "-";
            tallies.push_back(tally);
            found = tallies.size() - 1;
        }
        ++tallies[found].completed;
        if (appointment.date() > tallies[found].lastVisit) {
            tallies[found].lastVisit = appointment.date();
        }
    }

    // Highest attendance first; ties broken alphabetically.
    insertionSortRows(tallies, [](const Tally& left, const Tally& right) {
        if (left.completed != right.completed) {
            return left.completed > right.completed;
        }
        return left.name < right.name;
    });

    ReportResult report;
    report.title = name(ReportType::FrequentAttendees);
    report.description = "Patients ranked by the number of completed appointments.";
    report.headers = {"Rank", "Patient ID", "Patient Name", "Category", "Completed",
                      "Last Visit"};

    int rank = 0;
    int totalCompleted = 0;
    for (const auto& tally : tallies) {
        totalCompleted += tally.completed;
        if (rank >= 10) {
            continue; // top ten only
        }
        ++rank;
        report.rows.push_back({std::to_string(rank), tally.id, tally.name, tally.category,
                               std::to_string(tally.completed), tally.lastVisit});
    }

    report.summary.push_back("Patients with at least one completed appointment: " +
                             std::to_string(tallies.size()));
    report.summary.push_back("Total completed appointments: " +
                             std::to_string(totalCompleted));
    if (tallies.empty()) {
        report.summary.push_back("No appointment has been completed yet.");
    }
    return report;
}

ReportResult ReportGenerator::pendingAppointments() const {
    std::vector<model::Appointment> pending;
    for (const auto& appointment : m_database.appointments()) {
        if (appointment.isPending()) {
            pending.push_back(appointment);
        }
    }

    insertionSortRows(pending, [](const model::Appointment& left,
                                  const model::Appointment& right) {
        return left.startKey() < right.startKey();
    });

    ReportResult report;
    report.title = name(ReportType::PendingAppointments);
    report.description =
        "Appointments that have not yet been completed, earliest first.";
    report.headers = {"Appointment", "Patient ID", "Patient Name", "Date", "Time",
                      "Service", "Clinician", "Days Away"};

    const std::string today = scapms::utils::todayIso();
    int overdue = 0;
    for (const auto& appointment : pending) {
        const auto* patient = m_database.findPatientById(appointment.patientId());
        const int daysAway = scapms::utils::daysBetween(today, appointment.date());
        if (daysAway < 0) {
            ++overdue;
        }
        report.rows.push_back(
            {appointment.id(), appointment.patientId(),
             patient ? patient->name() : "(deleted patient)",
             appointment.date() + " (" + scapms::utils::weekdayName(appointment.date()) + ")",
             appointment.time(), appointment.serviceCode(), appointment.clinician(),
             (daysAway < 0 ? std::to_string(-daysAway) + " overdue"
                           : std::to_string(daysAway) + " day(s)")});
    }

    report.summary.push_back("Pending appointments: " + std::to_string(pending.size()));
    report.summary.push_back("Overdue (date already passed): " + std::to_string(overdue));
    if (pending.empty()) {
        report.summary.push_back("Every appointment has been completed or cancelled.");
    }
    return report;
}

ReportResult ReportGenerator::categoryDistribution() const {
    ReportResult report;
    report.title = name(ReportType::CategoryDistribution);
    report.description = "Breakdown of registered patients by patient category.";
    report.headers = {"Patient Category", "Patients", "Share",
                      "Total Appointments", "Completed", "Pending"};

    const std::vector<std::string> categories{"General", "Emergency", "Follow-Up"};
    const std::size_t total = m_database.patientCount();

    for (const auto& category : categories) {
        std::size_t patients = 0;
        std::size_t appointments = 0;
        std::size_t completed = 0;
        std::size_t pending = 0;

        for (const auto& record : m_database.patients()) {
            if (record && record->category() == category) {
                ++patients;
            }
        }
        for (const auto& appointment : m_database.appointments()) {
            const auto* patient = m_database.findPatientById(appointment.patientId());
            if (patient == nullptr || patient->category() != category) {
                continue;
            }
            ++appointments;
            if (appointment.isCompleted()) {
                ++completed;
            } else if (appointment.isPending()) {
                ++pending;
            }
        }

        report.rows.push_back({category, std::to_string(patients),
                               percent(patients, total), std::to_string(appointments),
                               std::to_string(completed), std::to_string(pending)});
    }

    report.summary.push_back("Total registered patients: " + std::to_string(total));
    report.summary.push_back("Total appointments: " +
                             std::to_string(m_database.appointmentCount()));
    return report;
}

ReportResult ReportGenerator::dailySummary(const std::string& date) const {
    ReportResult report;
    report.title = name(ReportType::DailySummary);
    report.description = "Appointment activity for " + date + " (" +
                         scapms::utils::weekdayName(date) + ").";
    report.headers = {"Appointment", "Patient", "Time", "Service", "Clinician",
                      "Status", "Fee"};

    std::vector<model::Appointment> appointments;
    for (const auto& appointment : m_database.appointments()) {
        if (appointment.date() == date) {
            appointments.push_back(appointment);
        }
    }
    insertionSortRows(appointments, [](const model::Appointment& left,
                                       const model::Appointment& right) {
        return left.time() < right.time();
    });

    int completed = 0;
    int pending = 0;
    int cancelled = 0;
    double collected = 0.0;
    double outstanding = 0.0;

    for (const auto& appointment : appointments) {
        const auto* patient = m_database.findPatientById(appointment.patientId());
        switch (appointment.status()) {
        case model::AppointmentStatus::Completed:
            ++completed;
            collected += appointment.fee();
            break;
        case model::AppointmentStatus::Pending:
            ++pending;
            outstanding += appointment.fee();
            break;
        case model::AppointmentStatus::Cancelled:
            ++cancelled;
            break;
        }
        report.rows.push_back(
            {appointment.id(), patient ? patient->name() : "(deleted patient)",
             appointment.time(), appointment.serviceCode(), appointment.clinician(),
             appointment.statusText(), scapms::utils::formatMoney(appointment.fee())});
    }

    report.summary.push_back("Total Appointments: " +
                             std::to_string(appointments.size()));
    report.summary.push_back("Completed: " + std::to_string(completed));
    report.summary.push_back("Pending: " + std::to_string(pending));
    report.summary.push_back("Cancelled: " + std::to_string(cancelled));
    report.summary.push_back("Fees collected: " +
                             scapms::utils::formatMoney(collected));
    report.summary.push_back("Fees outstanding: " +
                             scapms::utils::formatMoney(outstanding));
    if (appointments.empty()) {
        report.summary.push_back("No appointment is booked for this date.");
    }
    return report;
}

ReportResult ReportGenerator::revenueByService() const {
    struct ServiceTally {
        std::string code;
        std::string name;
        int bookings = 0;
        int completed = 0;
        double revenue = 0.0;
        double pendingValue = 0.0;
    };

    std::vector<ServiceTally> tallies;
    for (const auto& appointment : m_database.appointments()) {
        if (appointment.isCancelled()) {
            continue;
        }
        std::size_t found = tallies.size();
        for (std::size_t i = 0; i < tallies.size(); ++i) {
            if (tallies[i].code == appointment.serviceCode()) {
                found = i;
                break;
            }
        }
        if (found == tallies.size()) {
            ServiceTally tally;
            tally.code = appointment.serviceCode();
            const auto* service = m_database.findService(appointment.serviceCode());
            tally.name = service ? service->name() : appointment.serviceCode();
            tallies.push_back(tally);
            found = tallies.size() - 1;
        }
        ++tallies[found].bookings;
        if (appointment.isCompleted()) {
            ++tallies[found].completed;
            tallies[found].revenue += appointment.fee();
        } else {
            tallies[found].pendingValue += appointment.fee();
        }
    }

    insertionSortRows(tallies, [](const ServiceTally& left, const ServiceTally& right) {
        return left.revenue > right.revenue;
    });

    ReportResult report;
    report.title = name(ReportType::RevenueByService);
    report.description =
        "Income by medical service (cancelled appointments excluded).";
    report.headers = {"Service Code", "Service Name", "Bookings", "Completed",
                      "Revenue", "Pending Value"};

    double totalRevenue = 0.0;
    double totalPending = 0.0;
    for (const auto& tally : tallies) {
        totalRevenue += tally.revenue;
        totalPending += tally.pendingValue;
        report.rows.push_back({tally.code, tally.name, std::to_string(tally.bookings),
                               std::to_string(tally.completed),
                               scapms::utils::formatMoney(tally.revenue),
                               scapms::utils::formatMoney(tally.pendingValue)});
    }

    report.summary.push_back("Revenue collected: " +
                             scapms::utils::formatMoney(totalRevenue));
    report.summary.push_back("Revenue outstanding: " +
                             scapms::utils::formatMoney(totalPending));
    if (tallies.empty()) {
        report.summary.push_back("No billable appointment has been recorded.");
    }
    return report;
}

// =============================================================================
//  ReportResult formatting
// =============================================================================
std::string ReportResult::toPlainText() const {
    std::ostringstream out;
    out << title << '\n';
    out << std::string(title.size(), '=') << "\n\n";
    if (!description.empty()) {
        out << description << "\n\n";
    }

    std::vector<std::size_t> widths(headers.size(), 0);
    for (std::size_t i = 0; i < headers.size(); ++i) {
        widths[i] = headers[i].size();
    }
    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], row[i].size());
        }
    }

    for (std::size_t i = 0; i < headers.size(); ++i) {
        out << pad(headers[i], widths[i] + 2);
    }
    out << '\n';
    for (const auto width : widths) {
        out << std::string(width + 2, '-');
    }
    out << '\n';

    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            out << pad(row[i], widths[i] + 2);
        }
        out << '\n';
    }

    if (rows.empty()) {
        out << "(no records)\n";
    }

    if (!summary.empty()) {
        out << '\n';
        for (const auto& line : summary) {
            out << "  * " << line << '\n';
        }
    }
    return out.str();
}

std::string ReportResult::toCsv() const {
    std::ostringstream out;
    for (std::size_t i = 0; i < headers.size(); ++i) {
        if (i != 0) {
            out << ',';
        }
        out << scapms::utils::escapeCsv(headers[i]);
    }
    out << '\n';
    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i != 0) {
                out << ',';
            }
            out << scapms::utils::escapeCsv(row[i]);
        }
        out << '\n';
    }
    return out.str();
}

} // namespace scapms::controller
