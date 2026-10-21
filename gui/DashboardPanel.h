#pragma once
#include <QWidget>

#include "controller/ClinicManager.h"

class QGridLayout;
class QLabel;
class QListWidget;

namespace scapms::gui {

// "Dashboard" tab: several panels with live statistics, refreshed by a timer.
class DashboardPanel : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    QLabel* makeTile(QGridLayout* grid, int column, const QString& title, const QString& color);

    controller::ClinicManager& manager_;
    QLabel* patientsTile_ = nullptr;
    QLabel* pendingTile_ = nullptr;
    QLabel* completedTile_ = nullptr;
    QLabel* queueTile_ = nullptr;
    QLabel* categoryLabel_ = nullptr;
    QLabel* undoLabel_ = nullptr;
    QListWidget* reminders_ = nullptr;
    QListWidget* waitingAlerts_ = nullptr;
    QListWidget* frequent_ = nullptr;
    QLabel* clock_ = nullptr;
};

} // namespace scapms::gui
