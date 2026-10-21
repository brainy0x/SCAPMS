#include "gui/AdminPanel.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent) : QWidget(parent), manager_(manager) {
    // Layout choice: QGridLayout gives a 2x2 dashboard-style arrangement - undo box and
    // cache table on top, the wide activity log underneath spanning both columns.
    auto* root = new QGridLayout(this);

    auto* undoBox = new QGroupBox(tr("Undo Last Administrative Operation"));
    auto* undoLayout = new QVBoxLayout(undoBox);
    undoInfo_ = new QLabel();
    undoInfo_->setWordWrap(true);
    undoButton_ = new QPushButton(tr("Undo Last Operation"));
    undoButton_->setToolTip(tr("Reverse the most recent delete / modify / remove operation. Shortcut: Ctrl+Z"));
    undoLayout->addWidget(undoInfo_);
    undoLayout->addWidget(undoButton_, 0, Qt::AlignLeft);
    undoLayout->addStretch();

    auto* cacheBox = new QGroupBox(tr("Frequently Accessed Patient Records (cache of 10)"));
    auto* cacheLayout = new QVBoxLayout(cacheBox);
    cacheTable_ = new QTableWidget();
    configureTable(cacheTable_, {tr("Slot"), tr("ID"), tr("Name"), tr("Category"), tr("Accesses")});
    cacheLayout->addWidget(cacheTable_);

    auto* logBox = new QGroupBox(tr("Activity Log"));
    auto* logLayout = new QVBoxLayout(logBox);
    logList_ = new QListWidget();
    logLayout->addWidget(logList_);

    root->addWidget(undoBox, 0, 0);
    root->addWidget(cacheBox, 0, 1);
    root->addWidget(logBox, 1, 0, 1, 2);
    root->setColumnStretch(0, 1);
    root->setColumnStretch(1, 2);
    root->setRowStretch(1, 1);

    connect(undoButton_, &QPushButton::clicked, this, &AdminPanel::undoLast);
    refresh();
}

void AdminPanel::refresh() {
    undoButton_->setEnabled(manager_.canUndo());
    undoInfo_->setText(tr("Operations that can be undone: %1\nNext to undo: %2")
                           .arg(manager_.undoDepth())
                           .arg(qs(manager_.undoDescription())));

    const auto& cache = manager_.cache();
    cacheTable_->setRowCount(static_cast<int>(cache.size()));
    for (std::size_t i = 0; i < cache.size(); ++i) {
        const model::PatientRecord* p = cache.at(i);
        int row = static_cast<int>(i);
        QColor bg = categoryColor(p->category());
        cacheTable_->setItem(row, 0, makeItem(QString::number(row + 1), bg));
        cacheTable_->setItem(row, 1, makeItem(qs(p->id()), bg));
        cacheTable_->setItem(row, 2, makeItem(qs(p->name()), bg));
        cacheTable_->setItem(row, 3, makeItem(qs(p->category()), bg));
        cacheTable_->setItem(row, 4, makeItem(QString::number(cache.accessCount(i)), bg));
    }

    logList_->clear();
    const auto& log = manager_.activityLog();
    for (auto it = log.rbegin(); it != log.rend(); ++it) logList_->addItem(qs(*it));   // newest first
}

void AdminPanel::undoLast() {
    try {
        std::string what = manager_.undoLast();
        emit statusMessage(tr("Undone: %1").arg(qs(what)));
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showWarning(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while undoing."));
    }
}

} // namespace scapms::gui
