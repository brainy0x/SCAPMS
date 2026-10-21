#include "gui/PatientPanel.h"

#include <QComboBox>
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
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "model/PatientFactory.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::gui {

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    buildUi();
    rebuildExtraFields(categoryCombo_->currentText());
    onClear();
    refresh();
}

void PatientPanel::buildUi() {
    // Layout choice: a QHBoxLayout splits the tab into form (left) and table (right);
    // the form itself is a QFormLayout (label/field pairs line up automatically, and
    // rows can be added or removed at run time for the dynamic section); the buttons
    // sit in a QGridLayout so they stay in a tidy 2x2 block when the window is resized.
    auto* root = new QHBoxLayout(this);

    auto* formBox = new QGroupBox(tr("Patient Details"));
    auto* formBoxLayout = new QVBoxLayout(formBox);

    auto* form = new QFormLayout();
    idEdit_ = new QLineEdit();
    idEdit_->setToolTip(tr("Leave blank to generate an ID automatically. The ID cannot be changed later."));
    idWarning_ = new QLabel();
    idWarning_->setStyleSheet("color: #d32f2f;");
    nameEdit_ = new QLineEdit();
    nameEdit_->setToolTip(tr("Full name: letters, spaces, '-', '.' and apostrophes only."));
    ageEdit_ = new QLineEdit();
    ageEdit_->setToolTip(tr("Whole number between 0 and 120."));
    contactEdit_ = new QLineEdit();
    contactEdit_->setToolTip(tr("Phone number with 7 to 15 digits."));
    categoryCombo_ = new QComboBox();
    categoryCombo_->setToolTip(tr("Changing the category changes the extra fields below."));
    for (const auto& c : model::PatientFactory::categories()) categoryCombo_->addItem(qs(c));

    form->addRow(tr("Patient ID:"), idEdit_);
    form->addRow(QString(), idWarning_);
    form->addRow(tr("Name:"), nameEdit_);
    form->addRow(tr("Age:"), ageEdit_);
    form->addRow(tr("Contact:"), contactEdit_);
    form->addRow(tr("Category:"), categoryCombo_);
    formBoxLayout->addLayout(form);

    auto* extraBox = new QGroupBox(tr("Category-specific Details"));
    extraForm_ = new QFormLayout(extraBox);
    formBoxLayout->addWidget(extraBox);

    auto* buttons = new QGridLayout();
    addButton_ = new QPushButton(tr("Add Patient"));
    addButton_->setToolTip(tr("Register a new patient with the details above."));
    updateButton_ = new QPushButton(tr("Update Selected"));
    updateButton_->setToolTip(tr("Save the changes made to the selected patient (can be undone)."));
    deleteButton_ = new QPushButton(tr("Delete Selected"));
    deleteButton_->setToolTip(tr("Delete the selected patient and all their appointments (can be undone)."));
    clearButton_ = new QPushButton(tr("Clear Form"));
    clearButton_->setToolTip(tr("Empty the form and start a new registration."));
    buttons->addWidget(addButton_, 0, 0);
    buttons->addWidget(updateButton_, 0, 1);
    buttons->addWidget(deleteButton_, 1, 0);
    buttons->addWidget(clearButton_, 1, 1);
    formBoxLayout->addLayout(buttons);
    formBoxLayout->addStretch();

    auto* tableBox = new QGroupBox(tr("Registered Patients"));
    auto* tableLayout = new QVBoxLayout(tableBox);
    table_ = new QTableWidget();
    configureTable(table_, {tr("ID"), tr("Name"), tr("Age"), tr("Category"), tr("Priority"), tr("Contact"), tr("Pending")});
    table_->setToolTip(tr("Select a row to view or edit that patient."));
    tableLayout->addWidget(table_);

    root->addWidget(formBox, 2);
    root->addWidget(tableBox, 3);

    connect(addButton_, &QPushButton::clicked, this, &PatientPanel::onAdd);
    connect(updateButton_, &QPushButton::clicked, this, &PatientPanel::onUpdate);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::onDelete);
    connect(clearButton_, &QPushButton::clicked, this, &PatientPanel::onClear);
    connect(categoryCombo_, &QComboBox::currentTextChanged, this, &PatientPanel::onCategoryChanged);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &PatientPanel::onSelectionChanged);
    connect(idEdit_, &QLineEdit::textChanged, this, &PatientPanel::onIdEdited);       // text-field events
    connect(nameEdit_, &QLineEdit::textChanged, this, &PatientPanel::onNameEdited);
}

// Dynamic components: remove the old extra fields and create the ones for this category.
void PatientPanel::rebuildExtraFields(const QString& category) {
    while (extraForm_->rowCount() > 0) extraForm_->removeRow(0);
    extraEdits_.clear();
    try {
        for (const auto& label : model::PatientFactory::extraLabelsFor(ss(category))) {
            auto* edit = new QLineEdit();
            edit->setToolTip(tr("Specific to %1 patients.").arg(category));
            extraForm_->addRow(qs(label) + ":", edit);
            extraEdits_.push_back(edit);
        }
    } catch (...) {
        showError(this, tr("Could not build the form for category '%1'.").arg(category));
    }
}

void PatientPanel::onCategoryChanged(const QString& category) { rebuildExtraFields(category); }

void PatientPanel::refresh() {
    {
        QSignalBlocker blocker(table_);
        populatePatientTable(table_, manager_.database().patients());
        if (!editingId_.empty()) {
            int row = -1;
            for (int r = 0; r < table_->rowCount(); ++r)
                if (ss(table_->item(r, 0)->text()) == editingId_) row = r;
            if (row >= 0) table_->selectRow(row);
        }
    }
    if (!editingId_.empty() && !manager_.database().contains(editingId_)) onClear();
    if (editingId_.empty()) idEdit_->setPlaceholderText(qs(manager_.generatePatientId()) + tr(" (auto)"));
}

std::shared_ptr<model::PatientRecord> PatientPanel::readForm(const std::string& id) {
    int age = model::PatientRecord::parseAge(ss(ageEdit_->text()));
    std::vector<std::string> extras;
    for (auto* edit : extraEdits_) extras.push_back(ss(edit->text()));
    return model::PatientFactory::create(ss(categoryCombo_->currentText()), id, ss(nameEdit_->text()), age,
                                         ss(contactEdit_->text()), extras);
}

void PatientPanel::onAdd() {
    try {
        std::string id = utils::trim(ss(idEdit_->text()));
        if (id.empty()) id = manager_.generatePatientId();
        auto patient = readForm(id);
        manager_.addPatient(patient);
        emit statusMessage(tr("Registered %1 (%2).").arg(qs(patient->name()), qs(patient->id())));
        onClear();
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (const std::exception& e) {
        showError(this, tr("Unexpected problem: %1").arg(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while registering the patient."));
    }
}

void PatientPanel::onUpdate() {
    if (editingId_.empty()) {
        showWarning(this, tr("Select a patient in the table first."));
        return;
    }
    try {
        auto replacement = readForm(editingId_);
        manager_.updatePatient(editingId_, replacement);
        emit statusMessage(tr("Updated %1.").arg(qs(replacement->name())));
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (const std::exception& e) {
        showError(this, tr("Unexpected problem: %1").arg(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while updating the patient."));
    }
}

void PatientPanel::onDelete() {
    if (editingId_.empty()) {
        showWarning(this, tr("Select a patient in the table first."));
        return;
    }
    auto answer = QMessageBox::question(
        this, tr("Delete patient"),
        tr("Delete patient %1 and all their appointments?\n(You can undo this from the Admin tab or with Ctrl+Z.)")
            .arg(qs(editingId_)));
    if (answer != QMessageBox::Yes) return;
    try {
        manager_.deletePatient(editingId_);
        emit statusMessage(tr("Deleted patient %1.").arg(qs(editingId_)));
        onClear();
        emit dataChanged();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while deleting the patient."));
    }
}

void PatientPanel::onClear() {
    editingId_.clear();
    {
        QSignalBlocker blocker(table_);
        table_->clearSelection();
    }
    idEdit_->clear();
    nameEdit_->clear();
    ageEdit_->clear();
    contactEdit_->clear();
    for (auto* e : extraEdits_) e->clear();
    setEditing(QString());
    idEdit_->setPlaceholderText(qs(manager_.generatePatientId()) + tr(" (auto)"));
}

void PatientPanel::setEditing(const QString& id) {
    editingId_ = ss(id);
    bool editing = !id.isEmpty();
    idEdit_->setReadOnly(editing);
    addButton_->setEnabled(!editing);
    updateButton_->setEnabled(editing);
    deleteButton_->setEnabled(editing);
}

void PatientPanel::onSelectionChanged() {
    auto items = table_->selectedItems();
    if (items.isEmpty()) return;
    std::string id = ss(table_->item(items.first()->row(), 0)->text());
    try {
        auto patient = manager_.accessPatient(id);   // counts as an access for the array cache
        loadIntoForm(*patient);
        setEditing(qs(id));
        emit recordAccessed();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("Could not open the selected record."));
    }
}

void PatientPanel::loadIntoForm(const model::PatientRecord& patient) {
    idEdit_->setText(qs(patient.id()));
    nameEdit_->setText(qs(patient.name()));
    ageEdit_->setText(QString::number(patient.age()));
    contactEdit_->setText(qs(patient.contact()));
    {
        QSignalBlocker blocker(categoryCombo_);
        categoryCombo_->setCurrentText(qs(patient.category()));
    }
    rebuildExtraFields(qs(patient.category()));
    auto values = patient.extraValues();
    for (std::size_t i = 0; i < extraEdits_.size() && i < values.size(); ++i) extraEdits_[i]->setText(qs(values[i]));
}

void PatientPanel::onIdEdited(const QString& text) {
    std::string id = utils::trim(ss(text));
    bool duplicate = editingId_.empty() && !id.empty() && manager_.database().contains(id);
    idWarning_->setText(duplicate ? tr("Patient ID already exists.") : QString());
}

void PatientPanel::onNameEdited(const QString& text) {
    bool ok = true;
    if (!text.trimmed().isEmpty()) {
        try {
            model::PatientRecord::validateName(ss(text));
        } catch (...) {
            ok = false;
        }
    }
    nameEdit_->setStyleSheet(ok ? QString() : QStringLiteral("border: 1px solid #d32f2f;"));
}

} // namespace scapms::gui
