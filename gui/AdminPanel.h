#pragma once
#include <QWidget>

#include "controller/ClinicManager.h"

class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// "Admin" tab: Undo Last Administrative Operation (std::stack), activity log and the
// "Frequently Accessed Patient Records" array cache.
class AdminPanel : public QWidget {
    Q_OBJECT
public:
    explicit AdminPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();
    void undoLast();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private:
    controller::ClinicManager& manager_;
    QLabel* undoInfo_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QListWidget* logList_ = nullptr;
    QTableWidget* cacheTable_ = nullptr;
};

} // namespace scapms::gui
