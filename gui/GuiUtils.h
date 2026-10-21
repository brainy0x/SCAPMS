#pragma once
#include <memory>
#include <string>
#include <vector>

#include <QColor>
#include <QString>
#include <QStringList>

#include "model/Appointment.h"
#include "model/PatientRecord.h"

class QTableWidget;
class QTableWidgetItem;
class QWidget;

namespace scapms::gui {

inline QString qs(const std::string& s) { return QString::fromStdString(s); }
inline std::string ss(const QString& s) { return s.toStdString(); }

// Custom presentation: colour and label per patient category / appointment status.
QColor categoryColor(const std::string& category);
QColor statusColor(model::AppointmentStatus status);
QString priorityLabel(const std::string& category);

QTableWidgetItem* makeItem(const QString& text, const QColor& background = QColor(), bool bold = false);
void configureTable(QTableWidget* table, const QStringList& headers);
void populatePatientTable(QTableWidget* table, const std::vector<std::shared_ptr<model::PatientRecord>>& patients);

void showError(QWidget* parent, const QString& message);
void showWarning(QWidget* parent, const QString& message);
void showInfo(QWidget* parent, const QString& title, const QString& message);

} // namespace scapms::gui
