#include "gui/QueuePanel.h"

#include <ctime>

#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "utils/DateTimeUtils.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent) : QWidget(parent), manager_(manager) {
    buildUi();
    refresh();
}

void QueuePanel::buildUi() {
    // Layout choice: a QGridLayout places the controls (left, narrow) and the queue table
    // (right, wide) in one grid so both scale with the window; the control column is a
    // QVBoxLayout of group boxes.
    auto* root = new QGridLayout(this);

    auto* controls = new QVBoxLayout();

    auto* addBox = new QGroupBox(tr("Add to Queue"));
    auto* addLayout = new QVBoxLayout(addBox);
    patientCombo_ = new QComboBox();
    patientCombo_->setToolTip(tr("Choose a registered patient who has arrived."));
    auto* addButton = new QPushButton(tr("Add to Queue"));
    addButton->setToolTip(tr("Put the patient at the back of the waiting queue."));
    addLayout->addWidget(patientCombo_);
    addLayout->addWidget(addButton);
    controls->addWidget(addBox);

    auto* callBox = new QGroupBox(tr("Consultation"));
    auto* callLayout = new QVBoxLayout(callBox);
    auto* callButton = new QPushButton(tr("Call Next Patient"));
    callButton->setToolTip(tr("Remove the patient who has waited longest and call them in."));
    nowServing_ = new QLabel(tr("Nobody called yet."));
    nowServing_->setWordWrap(true);
    callLayout->addWidget(callButton);
    callLayout->addWidget(nowServing_);
    controls->addWidget(callBox);

    auto* alertBox = new QGroupBox(tr("Waiting-time Alert"));
    auto* alertLayout = new QFormLayout(alertBox);
    thresholdSpin_ = new QSpinBox();
    thresholdSpin_->setRange(1, 240);
    thresholdSpin_->setSuffix(tr(" min"));
    thresholdSpin_->setValue(manager_.waitAlertMinutes());
    thresholdSpin_->setToolTip(tr("Staff are alerted when someone has waited longer than this."));
    alertLayout->addRow(tr("Alert after:"), thresholdSpin_);
    controls->addWidget(alertBox);
    controls->addStretch();

    auto* queueBox = new QGroupBox(tr("Waiting Patients"));
    auto* queueLayout = new QVBoxLayout(queueBox);
    summary_ = new QLabel();
    table_ = new QTableWidget();
    configureTable(table_, {tr("#"), tr("Patient ID"), tr("Name"), tr("Category"), tr("Priority"), tr("Waiting")});
    auto* processButton = new QPushButton(tr("Process Queue (triage notes)"));
    processButton->setToolTip(tr("Handle every waiting patient through the common PatientRecord interface."));
    processOutput_ = new QTextEdit();
    processOutput_->setReadOnly(true);
    processOutput_->setMaximumHeight(110);
    processOutput_->setPlaceholderText(tr("Triage notes appear here."));
    queueLayout->addWidget(summary_);
    queueLayout->addWidget(table_, 1);
    queueLayout->addWidget(processButton, 0, Qt::AlignLeft);
    queueLayout->addWidget(processOutput_);

    root->addLayout(controls, 0, 0);
    root->addWidget(queueBox, 0, 1);
    root->setColumnStretch(0, 1);
    root->setColumnStretch(1, 3);

    connect(addButton, &QPushButton::clicked, this, &QueuePanel::onAdd);
    connect(callButton, &QPushButton::clicked, this, &QueuePanel::onCallNext);
    connect(processButton, &QPushButton::clicked, this, &QueuePanel::onProcess);
    connect(thresholdSpin_, &QSpinBox::valueChanged, this, &QueuePanel::onThresholdChanged);
}

void QueuePanel::refresh() {
    QString previous = patientCombo_->currentData().toString();
    {
        QSignalBlocker blocker(patientCombo_);
        patientCombo_->clear();
        for (const auto& p : manager_.database().patients())
            patientCombo_->addItem(qs(p->id() + " - " + p->name()), qs(p->id()));
        int index = patientCombo_->findData(previous);
        if (index >= 0) patientCombo_->setCurrentIndex(index);
    }

    auto entries = manager_.queueSnapshot();
    std::time_t now = std::time(nullptr);
    table_->setRowCount(static_cast<int>(entries.size()));
    int alerts = 0;
    for (int row = 0; row < static_cast<int>(entries.size()); ++row) {
        const auto& e = entries[static_cast<std::size_t>(row)];
        auto patient = manager_.database().find(e.patientId);
        std::string category = patient ? patient->category() : "General";
        long waited = static_cast<long>(now - e.joined);
        bool overdue = waited >= static_cast<long>(manager_.waitAlertMinutes()) * 60;
        if (overdue) ++alerts;
        QColor bg = overdue ? QColor("#ffe0b2") : categoryColor(category);
        table_->setItem(row, 0, makeItem(QString::number(row + 1), bg));
        table_->setItem(row, 1, makeItem(qs(e.patientId), bg));
        table_->setItem(row, 2, makeItem(patient ? qs(patient->name()) : QString(), bg));
        table_->setItem(row, 3, makeItem(qs(category), bg));
        table_->setItem(row, 4, makeItem(priorityLabel(category), bg));
        table_->setItem(row, 5, makeItem(qs(utils::formatDuration(waited)) + (overdue ? tr("  (long wait)") : QString()), bg, overdue));
    }
    summary_->setText(tr("%n patient(s) waiting", "", static_cast<int>(entries.size())) +
                      (alerts > 0 ? tr(" - %1 waiting longer than %2 min").arg(alerts).arg(manager_.waitAlertMinutes())
                                  : QString()));
}

void QueuePanel::onAdd() {
    try {
        std::string id = ss(patientCombo_->currentData().toString());
        if (id.empty()) throw utils::OperationException("Register a patient first, then add them to the queue.");
        manager_.enqueue(id);
        emit statusMessage(tr("%1 added to the waiting queue.").arg(qs(id)));
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while queueing the patient."));
    }
}

void QueuePanel::onCallNext() {
    try {
        auto patient = manager_.callNext();
        QString text = qs(controller::ClinicManager::processPatient(*patient));
        nowServing_->setText(tr("Now serving:\n%1").arg(text));
        emit statusMessage(tr("Calling %1.").arg(qs(patient->name())));
        showInfo(this, tr("Next patient"), text);
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while calling the next patient."));
    }
}

void QueuePanel::onProcess() {
    std::vector<controller::ClinicManager::PatientPtr> waiting;
    for (const auto& e : manager_.queueSnapshot())
        if (auto p = manager_.database().find(e.patientId)) waiting.push_back(p);
    if (waiting.empty()) {
        processOutput_->setPlainText(tr("Nobody is waiting."));
        return;
    }
    QStringList lines;
    int position = 1;
    for (const auto& line : controller::ClinicManager::processPatients(waiting))
        lines << QString("%1. %2").arg(position++).arg(qs(line));
    processOutput_->setPlainText(lines.join("\n"));
}

void QueuePanel::onThresholdChanged(int minutes) {
    manager_.setWaitAlertMinutes(minutes);
    refresh();
}

} // namespace scapms::gui
