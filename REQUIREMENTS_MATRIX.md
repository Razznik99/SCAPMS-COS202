# COS202 Lab Assessment — Requirement Traceability Matrix

**Case study:** Smart Clinic Appointment & Patient Management System (SCAPMS)
**Scope:** every requirement extracted from `Lab Assesment Questions.txt` / `lab-assessment-question-cos202.pdf`, mapped to the artefact that satisfies it.

Legend: **Status** — `DONE` implemented, `TEST` implemented + verified by `--selftest`, `GUI` implemented + verified in the running GUI.

---

## 1. Advanced Object-Oriented Programming

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 1a | Abstract class `PatientRecord` with Patient ID, Name, Age, Contact | `src/model/PatientRecord.h/.cpp` | DONE |
| 1a | At least one **pure virtual** function | `category()`, `priorityLevel()`, `consultationFee()`, `clinicalSummary()`, `clone()`, `typeTag()` are all `= 0` | DONE |
| 1b | ≥3 subclasses: `GeneralPatient`, `EmergencyPatient`, `FollowUpPatient` | `src/model/GeneralPatient.*`, `EmergencyPatient.*`, `FollowUpPatient.*` | DONE |
| 1b | Subclass behaviour appropriate to patient type | Fee calculation, triage priority, follow-up interval/discount, category label, clinical summary | DONE |
| 1c | Interface-like abstract class `Appointable` with `scheduleAppointment()`, `cancelAppointment()` | `src/model/Appointable.h`; implemented by `GeneralPatient` and `FollowUpPatient` (emergency patients are walk-in only) | DONE |
| 1d | Runtime polymorphism through base-class pointer/reference | `ClinicManager::describeAllPatients()`, GUI table built from `std::vector<PatientRecord*>` calling virtuals; verified by selftest | TEST |
| 1e | `ClinicDatabase` contains a collection of patient records | `src/model/ClinicDatabase.*` owns `std::vector<std::unique_ptr<PatientRecord>>` | DONE |
| 1e | `Appointment` class; a patient is associated with ≥1 appointment | `src/model/Appointment.*`, `ClinicDatabase::appointmentsForPatient()`; composition + association | DONE |
| 1f | Header/source organisation + namespaces | `src/model`, `src/controller`, `src/gui`, `src/utils`; namespaces `scapms::model`, `scapms::controller`, `scapms::gui`, `scapms::utils` | DONE |

## 2. Class Hierarchies & Program Organisation

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 2 | Logical components: Model / Controller / GUI / Utilities | 4 separate directories and namespaces; no GUI type is referenced by model or controller (pure C++ layers) | DONE |
| 2 | Named classes: ClinicManager, SearchEngine, AppointmentController, MainWindow, PatientPanel, AppointmentPanel, AdminPanel, SearchPanel, IDGenerator, FileHandler | All present with the requested names | DONE |
| 2 | GUI, business logic and data handling not fused into one class | `MainWindow` only wires signals to `ClinicManager`/controllers and renders results | DONE |

## 3. Data Structures

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 3a | `std::vector` of registered patients | `ClinicDatabase::m_patients` | DONE |
| 3b | `std::queue` waiting queue (first-in first-out) | `ClinicManager::m_normalQueue` (+ `m_priorityQueue` for triage override); `joinQueue`/`callNextPatient` | TEST |
| 3c | `std::stack` undo of last administrative operation (delete patient / remove appointment / modify record) | `ClinicManager::m_undoStack` of `std::unique_ptr<AdminCommand>`; 4 command classes | TEST |
| 3d | Fixed-size C++ array cache of frequently accessed patients | `PatientRecord* ClinicDatabase::m_recent[kCacheSize=10]` (MRU array, invalidated on delete) | TEST |

## 4. Searching Algorithms

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 4a | Hand-written **linear search** by Patient ID | `SearchEngine::linearSearchById()` | TEST |
| 4b | Hand-written **binary search** on a sorted field | `SearchEngine::binarySearchByName()` (+ `binarySearchById`) with student-written lower-bound | TEST |
| 4c | Recommended **recursive** search by name | `SearchEngine::recursiveSearchByName()` (recursive descent) | TEST |
| 4 | Appropriate technique chosen from data state (sorted/unsorted) | `SearchEngine::autoSearch()` — binary when sorted, linear otherwise; GUI shows the reason | TEST |

## 5. Sorting Algorithms

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 5 | Two hand-written sorting algorithms | Four: selection, insertion, merge (recursive), quick — `src/controller/SortEngine.*` | TEST |
| 5 | Sort by ≥2 fields | Four: name, ID, age, earliest appointment date | TEST |
| 5 | GUI combo box selects sorting algorithm (and field) | `SearchPanel` — "Sort Algorithm" + "Sort By" combo boxes + SORT button | GUI |
| 5 | Explain why the algorithm is appropriate | `SearchPanel` live rationale + report §sorting | DONE |

## 6. Event-Driven Programming

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 6a | Button events: Add, Delete, Schedule, Cancel, Search, Sort, Save, Load | All wired with `QObject::connect(..., &QPushButton::clicked, ...)` | GUI |
| 6b | Menu events: File → New / Save / Load / Exit | `MainWindow::createMenus()` + `QAction` slots | GUI |
| 6c | Text-field events: ID, name, search keywords, appointment info | `textChanged` → live validation styling, live search, `patientCombo` filtering | GUI |
| 6d | Timer events: reminders, waiting alerts, dashboard refresh | `QTimer` in `MainWindow` (1 s clock / 30 s refresh), `QueuePanel`, reminder banner | GUI |

## 7. GUI Programming, Components & Layout

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 7a | Patient Records tab: view/add/edit/delete | `PatientPanel` (`QTableWidget` + form + CRUD buttons) | GUI |
| 7b | Appointments tab: schedule/cancel/view | `AppointmentPanel` | GUI |
| 7c | Waiting Queue tab: view/add/call next | `QueuePanel` | GUI |
| 7d | Search & Sort tab: search, choose search algorithm, choose sort algorithm, sort | `SearchPanel` | GUI |
| 7 | Widgets: tables, buttons, text fields, combo boxes, labels, dialogs, menus, status info | All present; `QMessageBox`, `QFileDialog`, `QInputDialog`, `QStatusBar` | GUI |
| 8 | ≥2 layout techniques (not absolute positioning) | `QVBoxLayout`, `QHBoxLayout`, `QGridLayout`, `QFormLayout`, `QStackedWidget`, `QSplitter` — no `setGeometry`/`move` calls | DONE |
| 9 | ≥2 advanced GUI techniques | All seven: (a) custom `QStyledItemDelegate` colour/priority rendering, (b) dynamic type-specific form pages, (c) `QFileDialog` import/export, (d) timers, (e) validation dialogs, (f) shortcuts Ctrl+S/F/Q/Z/N/O/F5, (g) tooltips | GUI |

## 10. Exception Handling

| # | Requirement | Implementation | Status |
|---|-------------|----------------|--------|
| 10 | try/catch around risky operations | GUI slots catch `scapms::utils::ClinicException` + `std::exception`; controller validates before mutating | DONE |
| 10 | Invalid patient information | `ValidationException` in `ClinicManager::validatePatient()` | TEST |
| 10 | Duplicate patient ID | `DuplicateIdException` | TEST |
| 10 | Invalid age | `ValidationException` (0–120) | TEST |
| 10 | Invalid appointment data | `AppointmentException` (past date, unknown patient, bad time, conflict) | TEST |
| 10 | File loading failure | `FileException` (missing/corrupt file), shown as dialog, app keeps running | TEST |
| 10 | Invalid search input | `ValidationException` (empty keyword) | TEST |
| 10 | ≥1 custom exception class | 5 concrete exceptions deriving from `ClinicException : std::runtime_error` | DONE |
| 11 | Interactive / robust / error-tolerant / persistent (.txt/.csv/.json) | Full GUI, guarded operations, CSV persistence in `data/` | TEST |

## Mandatory System Features

| Feature | Where | Status |
|---------|-------|--------|
| Register new patients | PatientPanel + ClinicManager::registerPatient | GUI |
| Edit patient information | PatientPanel (double-click load + Update) | GUI |
| Delete patient records | PatientPanel + undoable command | GUI |
| Schedule appointments | AppointmentPanel | GUI |
| Cancel appointments | AppointmentPanel | GUI |
| Maintain waiting queue | QueuePanel (+ FIFO/priority) | GUI |
| Search by name, ID or category | SearchPanel (4 modes) | GUI |
| Sort with selectable algorithms | SearchPanel (4 algorithms × 4 fields) | GUI |
| ≥2 student-implemented sorts | Selection, Insertion, Merge, Quick | TEST |
| ≥2 student-implemented searches | Linear, Binary, Recursive | TEST |
| Recursive algorithm | Recursive name search + recursive merge sort | TEST |
| Undo via Stack | AdminPanel + Edit→Undo (Ctrl+Z) | TEST |
| GUI dashboard with multiple panels | Dashboard + 6 tabs | GUI |
| Save/load data files | File menu + AdminPanel, `data/*.csv` | TEST |
| Validation + exception handling | Controllers + dialog layer | TEST |
| Simple reports | ReportPanel (4 reports) + CSV export | GUI |

## Simple Reports

| # | Report | Implementation | Status |
|---|--------|----------------|--------|
| 1 | Most frequently attended patients (completed appointments) | `ReportGenerator::frequentAttendees()` | TEST |
| 2 | Patients with pending appointments | `ReportGenerator::pendingAppointments()` | TEST |
| 3 | Patient category distribution | `ReportGenerator::categoryDistribution()` | TEST |
| 4 | Daily appointment summary (total/completed/pending/cancelled) | `ReportGenerator::dailySummary()` | TEST |

## Submission Requirements (report produced after review sign-off)

| # | Item | Status |
|---|------|--------|
| 1 | Complete labelled source code | DONE — `src/` (model / controller / gui / utils), `tests/`, `resources/`, `data/`, `README.md` |
| 2 | Screenshots: dashboard, registration, appointments, search/sort, queue, reports | DONE — 7 PNGs in `screenshots/` produced by `SCAPMS.exe --capture` |
| 3 | UML/class diagram (inheritance, abstract, polymorphism, composition) | PENDING — generated with the report after sign-off |
| 4 | 2–3 page project report, 12 required sections | PENDING — written after sign-off, as requested |

## Verification Evidence

Environment: Qt 6.8.3 (MinGW 13.1 64-bit), Windows, CMake 4.4.4 / qmake 3.1.

| Check | Command | Result |
|-------|---------|--------|
| qmake build | `qmake ..\SCAPMS.pro && mingw32-make -j4` | clean build, **0 errors, 0 warnings** |
| CMake build | `cmake -S . -B build -G "MinGW Makefiles" && cmake --build build` | clean build |
| Automated tests | `SCAPMS.exe --selftest` | **279 checks passed, 0 failed**, exit code 0 |
| GUI runtime | `SCAPMS.exe --data data` | window opens; data seeds on first run; all seven tabs usable |
| Screenshot capture | `SCAPMS.exe --data data --capture screenshots` | 7 PNGs written (1875×1188) |

Manual verification covered: patient registration with auto ID, duplicate-ID rejection,
invalid-age and invalid-contact dialogs, editing and deleting a patient, Ctrl+Z restoration,
scheduling/conflict rejection, cancelling and completing appointments, adding to the queue,
calling the next patient (priority then FIFO), running search modes, running every sort
algorithm, generating every report, exporting a report to CSV, saving and reloading the
clinic, and the polymorphism demonstration.

**Executed verification log**

* `qmake` + `mingw32-make -j4`: clean build, no errors, no warnings.
* `cmake` + `cmake --build`: `[100%] Built target SCAPMS`.
* `SCAPMS.exe --selftest`: **279 checks passed, 0 failed**, exit code 0.
* GUI process stayed alive for the smoke window and closed cleanly through the window manager.
* UI Automation drove the real widgets: selecting the *Patient Records* tab exposed the form,
  and pressing **Add Patient** with an empty form surfaced the modal dialog
  **“Invalid patient information — Patient name must be between 2 and 60 characters.”**,
  after which the application remained usable (error-tolerant behaviour).
* `--capture` produced the seven screenshots referenced by the report.

## Warning: Student-Implemented Algorithms

No `std::sort`, `std::find`, `std::binary_search`, `std::lower_bound`, `std::stable_sort`, `std::priority_queue` or `std::map` ordering is used for the assessed algorithms. Searching and sorting are hand-written in `SearchEngine` / `SortEngine`; the only standard containers used are `vector`, `queue`, `stack` and a raw array, as the brief permits. Verified by `grep` + review.
