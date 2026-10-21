// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : tests/SelfTest.h
//  Purpose: Automated verification of the model, controller, algorithms,
//           undo stack, queue, persistence and reports. Run with:
//               SCAPMS.exe --selftest
// =============================================================================
#ifndef SCAPMS_TESTS_SELFTEST_H
#define SCAPMS_TESTS_SELFTEST_H

namespace scapms::tests {

/// Executes every test group. Returns the number of failed checks (0 = pass).
int runAllTests();

} // namespace scapms::tests

#endif // SCAPMS_TESTS_SELFTEST_H
