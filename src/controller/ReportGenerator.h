// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/controller/ReportGenerator.h
//  Layer  : Controller
//  Purpose: Produces the clinic's simple reports as tabular data that the GUI
//           renders and that can be exported to CSV.
// =============================================================================
#ifndef SCAPMS_CONTROLLER_REPORTGENERATOR_H
#define SCAPMS_CONTROLLER_REPORTGENERATOR_H

#include <string>
#include <vector>

#include "../model/ClinicDatabase.h"

namespace scapms::controller {

enum class ReportType {
    FrequentAttendees,
    PendingAppointments,
    CategoryDistribution,
    DailySummary,
    RevenueByService
};

struct ReportResult {
    std::string title;
    std::string description;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> summary;

    [[nodiscard]] bool isEmpty() const noexcept { return rows.empty(); }
    [[nodiscard]] std::string toPlainText() const;
    [[nodiscard]] std::string toCsv() const;
};

class ReportGenerator {
public:
    explicit ReportGenerator(const model::ClinicDatabase& database);

    /// Builds a report. `date` is only used by DailySummary (defaults to today).
    [[nodiscard]] ReportResult generate(ReportType type,
                                        const std::string& date = {}) const;

    [[nodiscard]] static std::string name(ReportType type);
    [[nodiscard]] static std::vector<ReportType> all();

private:
    [[nodiscard]] ReportResult frequentAttendees() const;
    [[nodiscard]] ReportResult pendingAppointments() const;
    [[nodiscard]] ReportResult categoryDistribution() const;
    [[nodiscard]] ReportResult dailySummary(const std::string& date) const;
    [[nodiscard]] ReportResult revenueByService() const;

    const model::ClinicDatabase& m_database;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_REPORTGENERATOR_H
