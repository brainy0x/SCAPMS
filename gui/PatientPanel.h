#pragma once
#include <memory>
#include <string>
#include <vector>

#include <QWidget>

#include "controller/ClinicManager.h"

class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// "Patient Records" tab: view / add / edit / delete patients. The category combo box
// drives a dynamic section of the form that shows the fields of the chosen patient type.
class PatientPanel : public QWidget {
    Q_OBJECT
public:
    explicit PatientPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void dataChanged();                  // the data was modified
    void recordAccessed();               // a record was only looked at (cache changed)
    void statusMessage(const QString& message);

private slots:
    void onAdd();
    void onUpdate();
    void onDelete();
    void onClear();
    void onCategoryChanged(const QString& category);
    void onSelectionChanged();
    void onIdEdited(const QString& text);
    void onNameEdited(const QString& text);

private:
    void buildUi();
    void rebuildExtraFields(const QString& category);
    std::shared_ptr<model::PatientRecord> readForm(const std::string& id);
    void loadIntoForm(const model::PatientRecord& patient);
    void setEditing(const QString& id);

    controller::ClinicManager& manager_;
    std::string editingId_;

    QLineEdit* idEdit_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* ageEdit_ = nullptr;
    QLineEdit* contactEdit_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    QFormLayout* extraForm_ = nullptr;
    std::vector<QLineEdit*> extraEdits_;
    QLabel* idWarning_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* updateButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace scapms::gui
