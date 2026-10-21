#pragma once
#include <set>
#include <string>

#include <QMainWindow>

#include "controller/ClinicManager.h"

class QLabel;
class QTabWidget;
class QTimer;

namespace scapms::gui {

class AdminPanel;
class AppointmentPanel;
class DashboardPanel;
class PatientPanel;
class QueuePanel;
class ReportPanel;
class SearchPanel;

// Main window: menus, tabs, status bar and the three timers. It wires the panels to the
// ClinicManager and to each other through signals and slots; it holds no business logic.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void newClinic();
    void loadFile();
    bool saveFile();
    bool saveFileAs();
    void undo();
    void focusSearch();
    void about();
    void onDataChanged();
    void refreshAll();
    void showStatus(const QString& message);
    void onDashboardTimer();
    void onReminderTimer();
    void onWaitingTimer();

private:
    void buildMenus();
    void buildTabs();
    void loadInitialData();
    bool confirmDiscardChanges();
    void setDirty(bool dirty);
    void updateStatusWidgets();
    void showAlert(const QString& title, const QString& text);

    controller::ClinicManager manager_;
    std::string currentFile_;
    bool dirty_ = false;

    QTabWidget* tabs_ = nullptr;
    DashboardPanel* dashboard_ = nullptr;
    PatientPanel* patients_ = nullptr;
    AppointmentPanel* appointments_ = nullptr;
    QueuePanel* queue_ = nullptr;
    SearchPanel* search_ = nullptr;
    ReportPanel* reports_ = nullptr;
    AdminPanel* admin_ = nullptr;

    QLabel* patientCountLabel_ = nullptr;
    QLabel* queueCountLabel_ = nullptr;
    QLabel* fileLabel_ = nullptr;
    QTimer* dashboardTimer_ = nullptr;
    QTimer* reminderTimer_ = nullptr;
    QTimer* waitingTimer_ = nullptr;
    std::set<std::string> remindedAppointments_;
    std::set<std::string> alertedPatients_;
};

} // namespace scapms::gui
