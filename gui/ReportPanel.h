#pragma once
#include <QWidget>

#include "controller/ClinicManager.h"

class QComboBox;
class QDateEdit;
class QTextEdit;

namespace scapms::gui {

// "Reports" tab: generate and export the clinic reports.
class ReportPanel : public QWidget {
    Q_OBJECT
public:
    explicit ReportPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();
    void exportReport();

signals:
    void statusMessage(const QString& message);

private slots:
    void generate();

private:
    controller::ClinicManager& manager_;
    QComboBox* reportCombo_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QTextEdit* output_ = nullptr;
};

} // namespace scapms::gui
