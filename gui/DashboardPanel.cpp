#include "gui/DashboardPanel.h"

#include <ctime>

#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "utils/DateTimeUtils.h"

namespace scapms::gui {

DashboardPanel::DashboardPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    // Layout choice: a QGridLayout holds the four statistic tiles in one row and the
    // information panels in a grid below, so everything stays aligned and resizes together.
    auto* root = new QGridLayout(this);

    clock_ = new QLabel();
    clock_->setStyleSheet("font-size: 12pt;");
    root->addWidget(clock_, 0, 0, 1, 4);

    patientsTile_ = makeTile(root, 0, tr("Registered Patients"), "#1565c0");
    pendingTile_ = makeTile(root, 1, tr("Pending Appointments"), "#ef6c00");
    completedTile_ = makeTile(root, 2, tr("Completed Appointments"), "#2e7d32");
    queueTile_ = makeTile(root, 3, tr("Patients Waiting"), "#c62828");

    auto* summaryBox = new QGroupBox(tr("Clinic Summary"));
    auto* summaryLayout = new QVBoxLayout(summaryBox);
    categoryLabel_ = new QLabel();
    undoLabel_ = new QLabel();
    summaryLayout->addWidget(categoryLabel_);
    summaryLayout->addWidget(undoLabel_);
    summaryLayout->addStretch();

    auto* reminderBox = new QGroupBox(tr("Appointment Reminders (next 60 minutes)"));
    auto* reminderLayout = new QVBoxLayout(reminderBox);
    reminders_ = new QListWidget();
    reminderLayout->addWidget(reminders_);

    auto* alertBox = new QGroupBox(tr("Waiting-time Alerts"));
    auto* alertLayout = new QVBoxLayout(alertBox);
    waitingAlerts_ = new QListWidget();
    alertLayout->addWidget(waitingAlerts_);

    auto* frequentBox = new QGroupBox(tr("Frequently Accessed Records"));
    auto* frequentLayout = new QVBoxLayout(frequentBox);
    frequent_ = new QListWidget();
    frequentLayout->addWidget(frequent_);

    root->addWidget(summaryBox, 2, 0, 1, 2);
    root->addWidget(frequentBox, 2, 2, 1, 2);
    root->addWidget(reminderBox, 3, 0, 1, 2);
    root->addWidget(alertBox, 3, 2, 1, 2);
    root->setRowStretch(2, 1);
    root->setRowStretch(3, 1);

    refresh();
}

QLabel* DashboardPanel::makeTile(QGridLayout* grid, int column, const QString& title, const QString& color) {
    auto* box = new QGroupBox(title);
    auto* layout = new QVBoxLayout(box);
    auto* value = new QLabel("0");
    value->setAlignment(Qt::AlignCenter);
    value->setStyleSheet(QString("font-size: 28pt; font-weight: bold; color: %1;").arg(color));
    layout->addWidget(value);
    grid->addWidget(box, 1, column);
    return value;
}

void DashboardPanel::refresh() {
    const auto& db = manager_.database();
    int pending = 0, completed = 0, general = 0, emergency = 0, followUp = 0;
    for (const auto& p : db.patients()) {
        pending += p->pendingCount();
        completed += p->completedCount();
        if (p->category() == "General") ++general;
        else if (p->category() == "Emergency") ++emergency;
        else ++followUp;
    }

    clock_->setText(tr("Today is %1").arg(QDateTime::currentDateTime().toString("dddd, d MMMM yyyy  HH:mm:ss")));
    patientsTile_->setText(QString::number(db.size()));
    pendingTile_->setText(QString::number(pending));
    completedTile_->setText(QString::number(completed));
    queueTile_->setText(QString::number(manager_.queueSize()));
    categoryLabel_->setText(tr("General: %1     Emergency: %2     Follow-Up: %3").arg(general).arg(emergency).arg(followUp));
    undoLabel_->setText(tr("Undo history: %1 operation(s). Next: %2")
                            .arg(manager_.undoDepth())
                            .arg(qs(manager_.undoDescription())));

    reminders_->clear();
    for (const auto& a : manager_.upcomingAppointments(60)) {
        auto patient = db.find(a.patientId());
        reminders_->addItem(qs(a.dateTime() + "  " + (patient ? patient->name() : a.patientId()) + " - " + a.reason()));
    }
    if (reminders_->count() == 0) reminders_->addItem(tr("No appointments in the next hour."));

    waitingAlerts_->clear();
    std::time_t now = std::time(nullptr);
    for (const auto& e : manager_.longWaiting(manager_.waitAlertMinutes())) {
        auto patient = db.find(e.patientId);
        waitingAlerts_->addItem(qs((patient ? patient->name() : e.patientId) + " has waited " +
                                   utils::formatDuration(static_cast<long>(now - e.joined))));
    }
    if (waitingAlerts_->count() == 0) waitingAlerts_->addItem(tr("Nobody has waited too long."));

    frequent_->clear();
    const auto& cache = manager_.cache();
    for (std::size_t i = 0; i < cache.size() && i < 5; ++i) {
        const model::PatientRecord* p = cache.at(i);
        frequent_->addItem(qs(p->name() + " (" + p->id() + ") - " + std::to_string(cache.accessCount(i)) + " access(es)"));
    }
    if (frequent_->count() == 0) frequent_->addItem(tr("No records accessed yet."));
}

} // namespace scapms::gui
