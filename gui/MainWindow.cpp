#include "gui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>

#include "gui/AdminPanel.h"
#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/GuiUtils.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportPanel.h"
#include "gui/SearchPanel.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("SCAPMS - Smart Clinic Appointment & Patient Management [*]"));
    resize(1100, 720);

    buildTabs();
    buildMenus();

    patientCountLabel_ = new QLabel();
    queueCountLabel_ = new QLabel();
    fileLabel_ = new QLabel();
    statusBar()->addPermanentWidget(patientCountLabel_);
    statusBar()->addPermanentWidget(queueCountLabel_);
    statusBar()->addPermanentWidget(fileLabel_);

    // Timer events: dashboard refresh (5 s), appointment reminders (30 s), waiting alerts (20 s).
    dashboardTimer_ = new QTimer(this);
    reminderTimer_ = new QTimer(this);
    waitingTimer_ = new QTimer(this);
    connect(dashboardTimer_, &QTimer::timeout, this, &MainWindow::onDashboardTimer);
    connect(reminderTimer_, &QTimer::timeout, this, &MainWindow::onReminderTimer);
    connect(waitingTimer_, &QTimer::timeout, this, &MainWindow::onWaitingTimer);
    dashboardTimer_->start(5000);
    reminderTimer_->start(30000);
    waitingTimer_->start(20000);

    loadInitialData();
    refreshAll();
    onReminderTimer();
    showStatus(tr("Ready."));
}

void MainWindow::buildTabs() {
    tabs_ = new QTabWidget();
    dashboard_ = new DashboardPanel(manager_);
    patients_ = new PatientPanel(manager_);
    appointments_ = new AppointmentPanel(manager_);
    queue_ = new QueuePanel(manager_);
    search_ = new SearchPanel(manager_);
    reports_ = new ReportPanel(manager_);
    admin_ = new AdminPanel(manager_);

    tabs_->addTab(dashboard_, tr("Dashboard"));
    tabs_->addTab(patients_, tr("Patient Records"));
    tabs_->addTab(appointments_, tr("Appointments"));
    tabs_->addTab(queue_, tr("Waiting Queue"));
    tabs_->addTab(search_, tr("Search && Sort"));
    tabs_->addTab(reports_, tr("Reports"));
    tabs_->addTab(admin_, tr("Admin"));
    setCentralWidget(tabs_);

    // Signals and slots: every panel reports changes; the window refreshes the others.
    connect(patients_, &PatientPanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(appointments_, &AppointmentPanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(queue_, &QueuePanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(search_, &SearchPanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(admin_, &AdminPanel::dataChanged, this, &MainWindow::onDataChanged);

    connect(patients_, &PatientPanel::recordAccessed, admin_, &AdminPanel::refresh);
    connect(patients_, &PatientPanel::recordAccessed, dashboard_, &DashboardPanel::refresh);
    connect(search_, &SearchPanel::recordAccessed, admin_, &AdminPanel::refresh);
    connect(search_, &SearchPanel::recordAccessed, dashboard_, &DashboardPanel::refresh);

    connect(patients_, &PatientPanel::statusMessage, this, &MainWindow::showStatus);
    connect(appointments_, &AppointmentPanel::statusMessage, this, &MainWindow::showStatus);
    connect(queue_, &QueuePanel::statusMessage, this, &MainWindow::showStatus);
    connect(search_, &SearchPanel::statusMessage, this, &MainWindow::showStatus);
    connect(reports_, &ReportPanel::statusMessage, this, &MainWindow::showStatus);
    connect(admin_, &AdminPanel::statusMessage, this, &MainWindow::showStatus);
}

void MainWindow::buildMenus() {
    // Menu events + keyboard shortcuts.
    QMenu* file = menuBar()->addMenu(tr("&File"));
    auto* newAction = file->addAction(tr("&New"), this, &MainWindow::newClinic);
    newAction->setShortcut(QKeySequence::New);
    auto* saveAction = file->addAction(tr("&Save"), this, &MainWindow::saveFile);
    saveAction->setShortcut(QKeySequence::Save);                              // Ctrl+S
    saveAction->setToolTip(tr("Save patients, appointments and the queue to the current file."));
    file->addAction(tr("Save &As..."), this, &MainWindow::saveFileAs);
    auto* loadAction = file->addAction(tr("&Load..."), this, &MainWindow::loadFile);
    loadAction->setShortcut(QKeySequence::Open);                              // Ctrl+O
    file->addSeparator();
    file->addAction(tr("&Export Report..."), reports_, &ReportPanel::exportReport);
    file->addSeparator();
    auto* exitAction = file->addAction(tr("E&xit"), this, &QWidget::close);
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));              // Ctrl+Q

    QMenu* edit = menuBar()->addMenu(tr("&Edit"));
    auto* undoAction = edit->addAction(tr("&Undo Last Administrative Operation"), this, &MainWindow::undo);
    undoAction->setShortcut(QKeySequence::Undo);                              // Ctrl+Z

    QMenu* searchMenu = menuBar()->addMenu(tr("&Search"));
    auto* findAction = searchMenu->addAction(tr("&Find Patient..."), this, &MainWindow::focusSearch);
    findAction->setShortcut(QKeySequence::Find);                              // Ctrl+F

    QMenu* help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About"), this, &MainWindow::about);
}

void MainWindow::loadInitialData() {
    // Look for the sample/saved data file beside the executable (or one folder above it).
    QString dir = QCoreApplication::applicationDirPath();
    QString path = dir + "/clinic_data.csv";
    if (!QFile::exists(path) && QFile::exists(dir + "/../clinic_data.csv")) path = dir + "/../clinic_data.csv";
    currentFile_ = ss(path);
    if (!QFile::exists(path)) return;
    try {
        manager_.load(currentFile_);
    } catch (const utils::ClinicException& e) {
        showWarning(this, tr("The data file could not be loaded:\n%1\n\nStarting with an empty clinic.").arg(qs(e.what())));
        manager_.clearAll();
    } catch (...) {
        showWarning(this, tr("The data file could not be loaded. Starting with an empty clinic."));
        manager_.clearAll();
    }
    setDirty(false);
}

// ---------------------------------------------------------------- file menu

bool MainWindow::confirmDiscardChanges() {
    if (!dirty_) return true;
    auto answer = QMessageBox::question(this, tr("Unsaved changes"), tr("Save the changes before continuing?"),
                                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) return saveFile();
    return answer == QMessageBox::Discard;
}

void MainWindow::newClinic() {
    if (!confirmDiscardChanges()) return;
    manager_.clearAll();
    remindedAppointments_.clear();
    alertedPatients_.clear();
    setDirty(false);
    refreshAll();
    showStatus(tr("Started a new, empty clinic."));
}

void MainWindow::loadFile() {
    if (!confirmDiscardChanges()) return;
    QString path = QFileDialog::getOpenFileName(this, tr("Load clinic data"), qs(currentFile_),
                                                tr("Clinic data (*.csv *.txt);;All files (*)"));
    if (path.isEmpty()) return;
    try {
        manager_.load(ss(path));
        currentFile_ = ss(path);
        remindedAppointments_.clear();
        alertedPatients_.clear();
        setDirty(false);
        refreshAll();
        showStatus(tr("Loaded %1.").arg(path));
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));     // memory is untouched when loading fails
    } catch (...) {
        showError(this, tr("The file could not be loaded."));
    }
}

bool MainWindow::saveFile() {
    if (currentFile_.empty()) return saveFileAs();
    try {
        manager_.save(currentFile_);
        setDirty(false);
        updateStatusWidgets();
        showStatus(tr("Saved to %1.").arg(qs(currentFile_)));
        return true;
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("The file could not be saved."));
    }
    return false;
}

bool MainWindow::saveFileAs() {
    QString path = QFileDialog::getSaveFileName(this, tr("Save clinic data"), qs(currentFile_),
                                                tr("CSV files (*.csv);;Text files (*.txt)"));
    if (path.isEmpty()) return false;
    currentFile_ = ss(path);
    return saveFile();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmDiscardChanges()) event->accept();
    else event->ignore();
}

// ----------------------------------------------------------------- slots

void MainWindow::undo() { admin_->undoLast(); }

void MainWindow::focusSearch() {
    tabs_->setCurrentWidget(search_);
    search_->focusSearch();
}

void MainWindow::about() {
    QMessageBox::about(this, tr("About SCAPMS"),
                       tr("<b>Smart Clinic Appointment &amp; Patient Management System</b><br>"
                          "COS202 Programming II - lab assessment project.<br><br>"
                          "Shortcuts: Ctrl+S save, Ctrl+O load, Ctrl+F search, Ctrl+Z undo, Ctrl+Q exit."));
}

void MainWindow::onDataChanged() {
    setDirty(true);
    refreshAll();
}

void MainWindow::refreshAll() {
    patients_->refresh();
    appointments_->refresh();
    queue_->refresh();
    search_->refresh();
    reports_->refresh();
    admin_->refresh();
    dashboard_->refresh();
    updateStatusWidgets();
}

void MainWindow::showStatus(const QString& message) { statusBar()->showMessage(message, 8000); }

void MainWindow::setDirty(bool dirty) {
    dirty_ = dirty;
    setWindowModified(dirty);
}

void MainWindow::updateStatusWidgets() {
    patientCountLabel_->setText(tr(" Patients: %1 ").arg(manager_.database().size()));
    queueCountLabel_->setText(tr(" Waiting: %1 ").arg(manager_.queueSize()));
    fileLabel_->setText(tr(" File: %1 ").arg(currentFile_.empty() ? tr("(none)") : QFileInfo(qs(currentFile_)).fileName()));
}

// ----------------------------------------------------------------- timers

void MainWindow::onDashboardTimer() {
    dashboard_->refresh();
    queue_->refresh();      // waiting times change as the clock runs
}

void MainWindow::showAlert(const QString& title, const QString& text) {
    auto* box = new QMessageBox(QMessageBox::Information, title, text, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setWindowModality(Qt::NonModal);   // never blocks the staff member's current work
    box->show();
}

void MainWindow::onReminderTimer() {
    for (const auto& a : manager_.upcomingAppointments(30)) {
        if (remindedAppointments_.count(a.id())) continue;
        remindedAppointments_.insert(a.id());
        auto patient = manager_.database().find(a.patientId());
        QString who = patient ? qs(patient->name()) : qs(a.patientId());
        showStatus(tr("Reminder: %1 at %2").arg(who, qs(a.time())));
        showAlert(tr("Appointment reminder"), tr("%1 has an appointment at %2 (%3).").arg(who, qs(a.dateTime()), qs(a.reason())));
    }
}

void MainWindow::onWaitingTimer() {
    auto late = manager_.longWaiting(manager_.waitAlertMinutes());
    std::set<std::string> stillWaiting;
    for (const auto& e : late) {
        stillWaiting.insert(e.patientId);
        if (alertedPatients_.count(e.patientId)) continue;
        alertedPatients_.insert(e.patientId);
        auto patient = manager_.database().find(e.patientId);
        QString who = patient ? qs(patient->name()) : qs(e.patientId);
        showStatus(tr("Alert: %1 has been waiting more than %2 minutes.").arg(who).arg(manager_.waitAlertMinutes()));
        showAlert(tr("Waiting-time alert"), tr("%1 has been waiting for more than %2 minutes.").arg(who).arg(manager_.waitAlertMinutes()));
    }
    for (auto it = alertedPatients_.begin(); it != alertedPatients_.end();)
        it = stillWaiting.count(*it) ? std::next(it) : alertedPatients_.erase(it);
}

} // namespace scapms::gui
