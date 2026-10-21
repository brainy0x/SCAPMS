#include "gui/AppointmentPanel.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    buildUi();
    refresh();
}

void AppointmentPanel::buildUi() {
    // Layout choice: QVBoxLayout stacks "scheduling form" above "appointment list";
    // QFormLayout aligns the booking fields; QHBoxLayout lines the action buttons up.
    auto* root = new QVBoxLayout(this);

    auto* bookBox = new QGroupBox(tr("Schedule an Appointment"));
    auto* bookLayout = new QVBoxLayout(bookBox);
    auto* form = new QFormLayout();
    patientCombo_ = new QComboBox();
    patientCombo_->setToolTip(tr("Choose the patient to book."));
    patientInfo_ = new QLabel();
    patientInfo_->setWordWrap(true);
    dateEdit_ = new QDateEdit(QDate::currentDate().addDays(1));
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setMinimumDate(QDate::currentDate());
    dateEdit_->setToolTip(tr("Appointment date (today or later)."));
    timeEdit_ = new QTimeEdit(QTime(9, 0));
    timeEdit_->setDisplayFormat("HH:mm");
    timeEdit_->setToolTip(tr("Appointment time (24-hour clock)."));
    reasonEdit_ = new QLineEdit();
    reasonEdit_->setPlaceholderText(tr("e.g. Blood pressure check"));
    reasonEdit_->setToolTip(tr("Short reason for the visit (required)."));
    form->addRow(tr("Patient:"), patientCombo_);
    form->addRow(QString(), patientInfo_);
    form->addRow(tr("Date:"), dateEdit_);
    form->addRow(tr("Time:"), timeEdit_);
    form->addRow(tr("Reason:"), reasonEdit_);
    bookLayout->addLayout(form);
    auto* scheduleButton = new QPushButton(tr("Schedule Appointment"));
    scheduleButton->setToolTip(tr("Book the appointment. Double-booked slots are rejected (except emergencies)."));
    bookLayout->addWidget(scheduleButton, 0, Qt::AlignLeft);

    auto* listBox = new QGroupBox(tr("Appointments"));
    auto* listLayout = new QVBoxLayout(listBox);
    auto* filterRow = new QHBoxLayout();
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({tr("All"), tr("Pending"), tr("Completed"), tr("Cancelled")});
    filterCombo_->setToolTip(tr("Show only appointments with this status."));
    filterRow->addWidget(new QLabel(tr("Show:")));
    filterRow->addWidget(filterCombo_);
    filterRow->addStretch();
    listLayout->addLayout(filterRow);

    table_ = new QTableWidget();
    configureTable(table_, {tr("Appointment"), tr("Patient ID"), tr("Patient"), tr("Date"), tr("Time"), tr("Reason"), tr("Status")});
    listLayout->addWidget(table_);

    auto* buttonRow = new QHBoxLayout();
    auto* cancelButton = new QPushButton(tr("Cancel Appointment"));
    cancelButton->setToolTip(tr("Mark the selected pending appointment as cancelled."));
    auto* completeButton = new QPushButton(tr("Mark Completed"));
    completeButton->setToolTip(tr("Mark the selected pending appointment as completed (the patient attended)."));
    auto* removeButton = new QPushButton(tr("Remove Appointment"));
    removeButton->setToolTip(tr("Delete the selected appointment record completely (can be undone)."));
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(completeButton);
    buttonRow->addWidget(removeButton);
    buttonRow->addStretch();
    listLayout->addLayout(buttonRow);

    root->addWidget(bookBox);
    root->addWidget(listBox, 1);

    connect(scheduleButton, &QPushButton::clicked, this, &AppointmentPanel::onSchedule);
    connect(cancelButton, &QPushButton::clicked, this, &AppointmentPanel::onCancel);
    connect(completeButton, &QPushButton::clicked, this, &AppointmentPanel::onComplete);
    connect(removeButton, &QPushButton::clicked, this, &AppointmentPanel::onRemove);
    connect(patientCombo_, &QComboBox::currentIndexChanged, this, [this](int) { onPatientChanged(); });
    connect(filterCombo_, &QComboBox::currentIndexChanged, this, [this](int) { refresh(); });
}

void AppointmentPanel::refresh() {
    QString previous = patientCombo_->currentData().toString();
    {
        QSignalBlocker blocker(patientCombo_);
        patientCombo_->clear();
        for (const auto& p : manager_.database().patients())
            patientCombo_->addItem(qs(p->id() + " - " + p->name()), qs(p->id()));
        int index = patientCombo_->findData(previous);
        if (index >= 0) patientCombo_->setCurrentIndex(index);
    }
    onPatientChanged();

    int filter = filterCombo_->currentIndex();   // 0 all, 1 pending, 2 completed, 3 cancelled
    std::vector<std::pair<std::string, model::Appointment>> rows;
    for (const auto& p : manager_.database().patients())
        for (const auto& a : p->appointments()) {
            if (filter == 1 && a.status() != model::AppointmentStatus::Pending) continue;
            if (filter == 2 && a.status() != model::AppointmentStatus::Completed) continue;
            if (filter == 3 && a.status() != model::AppointmentStatus::Cancelled) continue;
            rows.emplace_back(p->name(), a);
        }

    table_->setRowCount(static_cast<int>(rows.size()));
    int row = 0;
    for (const auto& r : rows) {
        const model::Appointment& a = r.second;
        QColor bg = statusColor(a.status());
        table_->setItem(row, 0, makeItem(qs(a.id()), bg));
        table_->setItem(row, 1, makeItem(qs(a.patientId()), bg));
        table_->setItem(row, 2, makeItem(qs(r.first), bg));
        table_->setItem(row, 3, makeItem(qs(a.date()), bg));
        table_->setItem(row, 4, makeItem(qs(a.time()), bg));
        table_->setItem(row, 5, makeItem(qs(a.reason()), bg));
        table_->setItem(row, 6, makeItem(qs(model::Appointment::statusToString(a.status())), bg,
                                         a.status() == model::AppointmentStatus::Pending));
        ++row;
    }
    dateEdit_->setMinimumDate(QDate::currentDate());
}

void AppointmentPanel::onPatientChanged() {
    std::string id = ss(patientCombo_->currentData().toString());
    auto patient = id.empty() ? nullptr : manager_.database().find(id);
    if (!patient) {
        patientInfo_->setText(tr("No patient selected."));
        return;
    }
    // Run-time polymorphism: the text depends on the real type behind the reference.
    patientInfo_->setText(qs(controller::ClinicManager::processPatient(*patient)));
    patientInfo_->setStyleSheet(QStringLiteral("color: palette(mid); font-style: italic;"));
}

QString AppointmentPanel::selectedAppointmentId() const {
    auto items = table_->selectedItems();
    if (items.isEmpty()) return QString();
    return table_->item(items.first()->row(), 0)->text();
}

void AppointmentPanel::onSchedule() {
    try {
        std::string patientId = ss(patientCombo_->currentData().toString());
        if (patientId.empty()) throw utils::InvalidAppointmentException("Choose a patient first.");
        auto appt = manager_.scheduleAppointment(patientId, ss(dateEdit_->date().toString("yyyy-MM-dd")),
                                                 ss(timeEdit_->time().toString("HH:mm")), ss(reasonEdit_->text()));
        reasonEdit_->clear();
        emit statusMessage(tr("Scheduled appointment %1 on %2.").arg(qs(appt.id()), qs(appt.dateTime())));
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (const std::exception& e) {
        showError(this, tr("Unexpected problem: %1").arg(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while scheduling."));
    }
}

void AppointmentPanel::runAction(void (controller::ClinicManager::*action)(const std::string&),
                                 const QString& doneMessage) {
    QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        showWarning(this, tr("Select an appointment in the table first."));
        return;
    }
    try {
        (manager_.*action)(ss(id));
        emit statusMessage(doneMessage.arg(id));
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred."));
    }
}

void AppointmentPanel::onCancel() {
    runAction(&controller::ClinicManager::cancelAppointment, tr("Cancelled appointment %1."));
}

void AppointmentPanel::onComplete() {
    runAction(&controller::ClinicManager::completeAppointment, tr("Appointment %1 marked as completed."));
}

void AppointmentPanel::onRemove() {
    QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        showWarning(this, tr("Select an appointment in the table first."));
        return;
    }
    auto answer = QMessageBox::question(this, tr("Remove appointment"),
                                        tr("Remove appointment %1 permanently?\n(It can be restored with Undo.)").arg(id));
    if (answer != QMessageBox::Yes) return;
    runAction(&controller::ClinicManager::removeAppointment, tr("Removed appointment %1."));
}

} // namespace scapms::gui
