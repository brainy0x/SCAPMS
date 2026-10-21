#include "gui/SearchPanel.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

using controller::SearchAlgorithm;
using controller::SearchField;
using controller::SortAlgorithm;
using controller::SortEngine;
using controller::SortField;

SearchPanel::SearchPanel(controller::ClinicManager& manager, QWidget* parent) : QWidget(parent), manager_(manager) {
    buildUi();
    refresh();
}

void SearchPanel::buildUi() {
    // Layout choice: QVBoxLayout stacks search box, sort box and results; each box uses a
    // QGridLayout so labels, combo boxes and buttons form aligned columns.
    auto* root = new QVBoxLayout(this);

    auto* searchBox = new QGroupBox(tr("Search"));
    auto* sg = new QGridLayout(searchBox);
    keywordEdit_ = new QLineEdit();
    keywordEdit_->setPlaceholderText(tr("Enter an ID, a name or a category"));
    keywordEdit_->setClearButtonEnabled(true);
    keywordEdit_->setToolTip(tr("Press Enter to search. Shortcut: Ctrl+F"));
    searchFieldCombo_ = new QComboBox();
    searchFieldCombo_->addItems({tr("Patient ID"), tr("Patient Name"), tr("Category")});
    searchFieldCombo_->setToolTip(tr("Which field to search."));
    searchAlgoCombo_ = new QComboBox();
    searchAlgoCombo_->addItems({tr("Auto (best for the data)"), tr("Linear Search"), tr("Binary Search"), tr("Recursive Search")});
    searchAlgoCombo_->setToolTip(tr("Binary Search needs the records sorted by the searched field."));
    auto* searchButton = new QPushButton(tr("Search"));
    auto* showAllButton = new QPushButton(tr("Show All"));
    searchInfo_ = new QLabel();
    searchInfo_->setWordWrap(true);
    sg->addWidget(new QLabel(tr("Keyword:")), 0, 0);
    sg->addWidget(keywordEdit_, 0, 1, 1, 3);
    sg->addWidget(new QLabel(tr("Search by:")), 1, 0);
    sg->addWidget(searchFieldCombo_, 1, 1);
    sg->addWidget(new QLabel(tr("Search algorithm:")), 1, 2);
    sg->addWidget(searchAlgoCombo_, 1, 3);
    sg->addWidget(searchButton, 2, 0, 1, 2);
    sg->addWidget(showAllButton, 2, 2, 1, 2);
    sg->addWidget(searchInfo_, 3, 0, 1, 4);

    auto* sortBox = new QGroupBox(tr("Sort"));
    auto* so = new QGridLayout(sortBox);
    sortAlgoCombo_ = new QComboBox();
    sortAlgoCombo_->addItems({tr("Selection Sort"), tr("Insertion Sort"), tr("Merge Sort"), tr("Quick Sort")});
    sortAlgoCombo_->setCurrentIndex(2);
    sortAlgoCombo_->setToolTip(tr("The sorting algorithm that will be used."));
    sortFieldCombo_ = new QComboBox();
    sortFieldCombo_->addItems({tr("Patient Name"), tr("Patient ID"), tr("Age"), tr("Appointment Date")});
    sortFieldCombo_->setToolTip(tr("The field the records are ordered by."));
    auto* sortButton = new QPushButton(tr("SORT"));
    sortButton->setToolTip(tr("Reorder all patient records."));
    orderLabel_ = new QLabel();
    sortExplain_ = new QLabel();
    sortExplain_->setWordWrap(true);
    sortExplain_->setStyleSheet(QStringLiteral("color: palette(mid);"));
    so->addWidget(new QLabel(tr("Sort algorithm:")), 0, 0);
    so->addWidget(sortAlgoCombo_, 0, 1);
    so->addWidget(new QLabel(tr("Sort by:")), 0, 2);
    so->addWidget(sortFieldCombo_, 0, 3);
    so->addWidget(sortButton, 0, 4);
    so->addWidget(orderLabel_, 1, 0, 1, 5);
    so->addWidget(sortExplain_, 2, 0, 1, 5);
    so->setColumnStretch(1, 1);
    so->setColumnStretch(3, 1);

    auto* resultBox = new QGroupBox(tr("Results"));
    auto* rl = new QVBoxLayout(resultBox);
    resultInfo_ = new QLabel();
    table_ = new QTableWidget();
    configureTable(table_, {tr("ID"), tr("Name"), tr("Age"), tr("Category"), tr("Priority"), tr("Contact"), tr("Pending")});
    rl->addWidget(resultInfo_);
    rl->addWidget(table_);

    root->addWidget(searchBox);
    root->addWidget(sortBox);
    root->addWidget(resultBox, 1);

    connect(searchButton, &QPushButton::clicked, this, &SearchPanel::onSearch);
    connect(keywordEdit_, &QLineEdit::returnPressed, this, &SearchPanel::onSearch);
    connect(keywordEdit_, &QLineEdit::textChanged, this, &SearchPanel::onKeywordEdited);
    connect(showAllButton, &QPushButton::clicked, this, &SearchPanel::onShowAll);
    connect(sortButton, &QPushButton::clicked, this, &SearchPanel::onSort);
    connect(sortAlgoCombo_, &QComboBox::currentIndexChanged, this, [this](int) { onSortChoiceChanged(); });
    onSortChoiceChanged();
}

void SearchPanel::focusSearch() {
    keywordEdit_->setFocus();
    keywordEdit_->selectAll();
}

void SearchPanel::refresh() {
    showPatients(manager_.database().patients());
    updateOrderLabel();
    onSortChoiceChanged();
}

void SearchPanel::showPatients(const std::vector<controller::ClinicManager::PatientPtr>& patients) {
    populatePatientTable(table_, patients);
}

void SearchPanel::updateOrderLabel() {
    auto order = manager_.currentOrder();
    orderLabel_->setText(order ? tr("Records are currently sorted by: %1").arg(qs(SortEngine::fieldName(*order)))
                               : tr("Records are currently in registration order (not sorted)."));
}

void SearchPanel::onKeywordEdited(const QString& text) {
    // Text-field event: give immediate feedback about what will be searched.
    searchInfo_->setText(text.trimmed().isEmpty() ? QString() : tr("Press Enter or click Search to look for '%1'.").arg(text.trimmed()));
}

void SearchPanel::onSearch() {
    try {
        SearchField field = static_cast<SearchField>(searchFieldCombo_->currentIndex());
        SearchAlgorithm algo = static_cast<SearchAlgorithm>(searchAlgoCombo_->currentIndex());
        auto result = manager_.search(field, algo, ss(keywordEdit_->text()));
        showPatients(result.matches);
        resultInfo_->setText(tr("%1 match(es) found using %2.").arg(result.matches.size()).arg(qs(result.algorithmUsed)));
        searchInfo_->setText(qs(result.note));
        emit statusMessage(tr("%1: %2 match(es).").arg(qs(result.algorithmUsed)).arg(result.matches.size()));
        emit recordAccessed();
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while searching."));
    }
}

void SearchPanel::onShowAll() {
    showPatients(manager_.database().patients());
    resultInfo_->setText(tr("Showing all %1 patient(s).").arg(manager_.database().size()));
}

void SearchPanel::onSort() {
    try {
        SortAlgorithm algo = static_cast<SortAlgorithm>(sortAlgoCombo_->currentIndex());
        // The combo lists Name first, but the SortField enum starts with Id, so map explicitly.
        static const SortField comboToField[] = {SortField::Name, SortField::Id, SortField::Age, SortField::AppointmentDate};
        SortField field = comboToField[sortFieldCombo_->currentIndex()];

        auto stats = manager_.sort(algo, field);
        emit dataChanged();   // refresh() of every tab re-shows the sorted list
        resultInfo_->setText(tr("Sorted %1 record(s) by %2 using %3: %4 comparisons, %5 moves, %6 ms.")
                                 .arg(manager_.database().size())
                                 .arg(qs(SortEngine::fieldName(field)))
                                 .arg(qs(SortEngine::algorithmName(algo)))
                                 .arg(stats.comparisons)
                                 .arg(stats.moves)
                                 .arg(stats.milliseconds, 0, 'f', 3));
        emit statusMessage(tr("Sorted by %1 with %2.").arg(qs(SortEngine::fieldName(field)), qs(SortEngine::algorithmName(algo))));
    } catch (const utils::ClinicException& e) {
        showError(this, qs(e.what()));
    } catch (...) {
        showError(this, tr("An unknown error occurred while sorting."));
    }
}

void SearchPanel::onSortChoiceChanged() {
    SortAlgorithm algo = static_cast<SortAlgorithm>(sortAlgoCombo_->currentIndex());
    sortExplain_->setText(tr("Why %1? %2")
                              .arg(qs(SortEngine::algorithmName(algo)),
                                   qs(SortEngine::explain(algo, manager_.database().size()))));
}

} // namespace scapms::gui
