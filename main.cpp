#include <QApplication>
#include <QMessageBox>

#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setApplicationName("SCAPMS");

    try {
        scapms::gui::MainWindow window;
        window.show();
        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "SCAPMS", QString("The application could not start:\n%1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(nullptr, "SCAPMS", "The application could not start because of an unknown error.");
    }
    return 1;
}
