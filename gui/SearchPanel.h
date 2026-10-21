#pragma once
#include <QWidget>

#include "controller/ClinicManager.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;

namespace scapms::gui {

// "Search & Sort" tab: choose a search algorithm / field, choose a sort algorithm / field.
class SearchPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);
    void focusSearch();

public slots:
    void refresh();

signals:
    void dataChanged();
    void recordAccessed();
    void statusMessage(const QString& message);

private slots:
    void onSearch();
    void onShowAll();
    void onSort();
    void onSortChoiceChanged();
    void onKeywordEdited(const QString& text);

private:
    void buildUi();
    void showPatients(const std::vector<controller::ClinicManager::PatientPtr>& patients);
    void updateOrderLabel();

    controller::ClinicManager& manager_;
    QLineEdit* keywordEdit_ = nullptr;
    QComboBox* searchFieldCombo_ = nullptr;
    QComboBox* searchAlgoCombo_ = nullptr;
    QComboBox* sortAlgoCombo_ = nullptr;
    QComboBox* sortFieldCombo_ = nullptr;
    QLabel* searchInfo_ = nullptr;
    QLabel* sortExplain_ = nullptr;
    QLabel* orderLabel_ = nullptr;
    QLabel* resultInfo_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace scapms::gui
