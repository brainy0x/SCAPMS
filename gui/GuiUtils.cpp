#include "gui/GuiUtils.h"

#include <QBrush>
#include <QFont>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>

namespace scapms::gui {

QColor categoryColor(const std::string& category) {
    if (category == "Emergency") return QColor("#ffcdd2");   // red: high priority
    if (category == "Follow-Up") return QColor("#bbdefb");   // blue: scheduled
    return QColor("#c8e6c9");                                // green: normal
}

QColor statusColor(model::AppointmentStatus status) {
    switch (status) {
    case model::AppointmentStatus::Pending: return QColor("#fff3c4");     // amber
    case model::AppointmentStatus::Completed: return QColor("#c8e6c9");   // green
    case model::AppointmentStatus::Cancelled: return QColor("#e0e0e0");   // grey
    }
    return QColor();
}

QString priorityLabel(const std::string& category) {
    if (category == "Emergency") return QStringLiteral("High Priority");
    if (category == "Follow-Up") return QStringLiteral("Scheduled");
    return QStringLiteral("Normal");
}

QTableWidgetItem* makeItem(const QString& text, const QColor& background, bool bold) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    if (background.isValid()) {
        item->setBackground(QBrush(background));
        item->setForeground(QBrush(Qt::black));   // keep text readable on pastel backgrounds in any theme
    }
    if (bold) {
        QFont f = item->font();
        f.setBold(true);
        item->setFont(f);
    }
    return item;
}

void configureTable(QTableWidget* table, const QStringList& headers) {
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
}

void populatePatientTable(QTableWidget* table, const std::vector<std::shared_ptr<model::PatientRecord>>& patients) {
    table->setRowCount(static_cast<int>(patients.size()));
    int row = 0;
    for (const auto& p : patients) {
        QColor bg = categoryColor(p->category());
        bool urgent = p->category() == "Emergency";
        table->setItem(row, 0, makeItem(qs(p->id()), bg, urgent));
        table->setItem(row, 1, makeItem(qs(p->name()), bg, urgent));
        table->setItem(row, 2, makeItem(QString::number(p->age()), bg, urgent));
        table->setItem(row, 3, makeItem(qs(p->category()), bg, urgent));
        table->setItem(row, 4, makeItem(priorityLabel(p->category()), bg, urgent));
        table->setItem(row, 5, makeItem(qs(p->contact()), bg, urgent));
        table->setItem(row, 6, makeItem(QString::number(p->pendingCount()), bg, urgent));
        ++row;
    }
}

void showError(QWidget* parent, const QString& message) {
    QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Error: %1").arg(message));
}

void showWarning(QWidget* parent, const QString& message) {
    QMessageBox::warning(parent, QObject::tr("Warning"), message);
}

void showInfo(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox::information(parent, title, message);
}

} // namespace scapms::gui
