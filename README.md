# SCAPMS - Smart Clinic Appointment & Patient Management System

COS202 Programming II lab assessment. C++17 + Qt 6 (Widgets), built with CMake.

## Build (Windows)

1. Install Qt 6.3 or newer with the **MinGW** kit and **CMake/Ninja** (Qt Online Installer -> Qt 6.x -> MinGW 64-bit, plus Developer and Designer Tools -> CMake, Ninja).
2. Open `CMakeLists.txt` in **Qt Creator**, choose the MinGW kit, press Run. Or from a Qt command prompt:
   ```
   cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/mingw_64
   cmake --build build
   build\SCAPMS.exe
   ```
3. `data/clinic_data.csv` is copied next to the executable on the first build and loaded at start-up.
   If you launch the exe outside Qt Creator, run `windeployqt build\SCAPMS.exe` once.

## Layout

| Folder | Contents |
|---|---|
| `model/` | `PatientRecord` (abstract), `GeneralPatient`, `EmergencyPatient`, `FollowUpPatient`, `Appointable` (interface), `Appointment`, `ClinicDatabase`, `PatientFactory`, `QueueEntry` |
| `controller/` | `ClinicManager` (queue, undo stack, cache), `SearchEngine`, `SortEngine`, `AppointmentController`, `ReportGenerator`, `RecentPatientCache`, `AdminOperation` |
| `gui/` | `MainWindow`, `DashboardPanel`, `PatientPanel`, `AppointmentPanel`, `QueuePanel`, `SearchPanel`, `ReportPanel`, `AdminPanel`, `GuiUtils` |
| `utils/` | `Exceptions.h`, `IDGenerator.h`, `FileHandler`, `StringUtils.h`, `DateTimeUtils.h` |
| `data/` | `clinic_data.csv` sample data |

Namespaces: `scapms::model`, `scapms::controller`, `scapms::gui`, `scapms::utils`. Only `gui/` and `main.cpp` use Qt.

## Where each requirement lives

| Requirement | Where |
|---|---|
| Abstract class + pure virtuals | `model/PatientRecord.h` |
| Interface | `model/Appointable.h` |
| Runtime polymorphism | `ClinicManager::processPatient` / `processPatients` (Queue tab "Process Queue", Appointments tab info line) |
| Composition | `ClinicDatabase` -> `PatientRecord` -> `Appointment` |
| `std::vector` / `std::queue` / `std::stack` / array | `ClinicDatabase`, `ClinicManager::waiting_`, `ClinicManager::undoStack_`, `RecentPatientCache` |
| Linear, binary, recursive search | `controller/SearchEngine.cpp`; Auto mode picks by data state in `ClinicManager::search` |
| Selection, insertion, merge, quick sort | `controller/SortEngine.h`; explanation text in `SortEngine::explain` |
| Events, menus, timers, shortcuts, tooltips | `gui/MainWindow.cpp` and panels |
| Layouts | QVBox / QHBox / QGrid / QForm in every panel (reason noted in a comment in each) |
| Custom table colours, dynamic form, file dialog, validation dialogs | `GuiUtils.cpp`, `PatientPanel.cpp`, `MainWindow.cpp`, `utils/Exceptions.h` |
| Reports | `controller/ReportGenerator.cpp` (5 reports) |

Shortcuts: Ctrl+S save, Ctrl+O load, Ctrl+F search, Ctrl+Z undo, Ctrl+Q exit, Ctrl+N new.
