// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/controller/DemoData.h
//  Layer  : Controller
//  Purpose: Seeds a realistic demonstration clinic so that a fresh checkout can
//           be explored (and screenshotted) immediately. Dates are generated
//           relative to "today" so the dashboard and reports always look alive.
// =============================================================================
#ifndef SCAPMS_CONTROLLER_DEMODATA_H
#define SCAPMS_CONTROLLER_DEMODATA_H

namespace scapms::controller {

class ClinicManager;

class DemoData {
public:
    /// Fills an empty clinic with 14 patients, 25 appointments and a waiting
    /// queue. Does nothing when data already exists.
    static void seed(ClinicManager& manager);
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_DEMODATA_H
