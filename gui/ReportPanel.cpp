#include "gui/ReportPanel.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "controller/ReportGenerator.h"
#include "gui/GuiUtils.h"
#include "utils/Exceptions.h"
#include "utils/FileHandler.h"

namespace scapms::gui {

ReportPanel::ReportPanel(controller::ClinicManager& manager, QWidget* parent) : QWidget(parent), manager_(manager) {
    // Layout choice: QVBoxLayout (toolbar row above a large text area) - the report text
    // simply takes all remaining space; QHBoxLayout keeps the toolbar controls in one row.
    auto* root = new QVBoxLayout(this);
    auto* bar = new QHBoxLayout();

    reportCombo_ = new QComboBox();
    reportCombo_->addItems({tr("Most Frequently Attended Patients"), tr("Patients with Pending Appointments"),
                            tr("Patient Category Distribution"), tr("Daily Appointment Summary"),
                            tr("Waiting Queue Status"), tr("All Reports")});
    reportCombo_->setToolTip(tr("Choose which report to display."));
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip(tr("Day used by the Daily Appointment Summary."));
    auto* generateButton = new QPushButton(tr("Generate"));
    auto* exportButton = new QPushButton(tr("Export to File..."));
    exportButton->setToolTip(tr("Save the displayed report as a text file."));

    bar->addWidget(new QLabel(tr("Report:")));
    bar->addWidget(reportCombo_, 1);
    bar->addWidget(new QLabel(tr("Date:")));
    bar->addWidget(dateEdit_);
    bar->addWidget(generateButton);
    bar->addWidget(exportButton);

    output_ = new QTextEdit();
    output_->setReadOnly(true);
    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    output_->setFont(mono);

    root->addLayout(bar);
    root->addWidget(output_, 1);

    connect(generateButton, &QPushButton::clicked, this, &ReportPanel::generate);
    connect(exportButton, &QPushButton::clicked, this, &ReportPanel::exportReport);
    connect(reportCombo_, &QComboBox::currentIndexChanged, this, [this](int) { generate(); });
    connect(dateEdit_, &QDateEdit::dateChanged, this, [this](const QDate&) { generate(); });
    generate();
}

void ReportPanel::refresh() { generate(); }

void ReportPanel::generate() {
    using controller::ReportGenerator;
    try {
        const auto& db = manager_.database();
        std::string date = ss(dateEdit_->date().toString("yyyy-MM-dd"));
        std::string text;
        switch (reportCombo_->currentIndex()) {
        case 0: text = ReportGenerator::mostFrequentlyAttended(db); break;
        case 1: text = ReportGenerator::pendingAppointments(db); break;
        case 2: text = ReportGenerator::categoryDistribution(db); break;
        case 3: text = ReportGenerator::dailySummary(db, date); break;
        case 4: text = ReportGenerator::waitingQueue(db, manager_.queueSnapshot()); break;
        default: text = ReportGenerator::all(db, manager_.queueSnapshot(), date); break;
        }
        output_->setPlainText(qs(text));
    } catch (...) {
        showError(this, tr("The report could not be generated."));
    }
}

void ReportPanel::exportReport() {
    QString path = QFileDialog::getSaveFileName(this, tr("Export report"), "clinic_report.txt",
                                                tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty()) return;
    try {
        utils::FileHandler::writeText(ss(path), ss(output_->toPlainText()));
        emit statusMessage(tr("Report exported to %1.").arg(path));
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("The report could not be exported."));
    }
}

} // namespace scapms::gui
