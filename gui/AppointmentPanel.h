#pragma once
#include <QWidget>

#include "controller/ClinicManager.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimeEdit;

namespace scapms::gui {

// "Appointments" tab: schedule, cancel, complete, remove and view appointments.
class AppointmentPanel : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void onSchedule();
    void onCancel();
    void onComplete();
    void onRemove();
    void onPatientChanged();

private:
    void buildUi();
    QString selectedAppointmentId() const;
    void runAction(void (controller::ClinicManager::*action)(const std::string&), const QString& doneMessage);

    controller::ClinicManager& manager_;
    QComboBox* patientCombo_ = nullptr;
    QLabel* patientInfo_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QTimeEdit* timeEdit_ = nullptr;
    QLineEdit* reasonEdit_ = nullptr;
    QComboBox* filterCombo_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace scapms::gui
