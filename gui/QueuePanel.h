#pragma once
#include <QWidget>

#include "controller/ClinicManager.h"

class QComboBox;
class QLabel;
class QSpinBox;
class QTableWidget;
class QTextEdit;

namespace scapms::gui {

// "Waiting Queue" tab: first-come-first-served queue (std::queue in ClinicManager).
class QueuePanel : public QWidget {
    Q_OBJECT
public:
    explicit QueuePanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void onAdd();
    void onCallNext();
    void onProcess();
    void onThresholdChanged(int minutes);

private:
    void buildUi();

    controller::ClinicManager& manager_;
    QComboBox* patientCombo_ = nullptr;
    QSpinBox* thresholdSpin_ = nullptr;
    QLabel* summary_ = nullptr;
    QLabel* nowServing_ = nullptr;
    QTableWidget* table_ = nullptr;
    QTextEdit* processOutput_ = nullptr;
};

} // namespace scapms::gui
