#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    QString styleSheet = R"(
        QWidget {
            background-color: #121212;
            color: #E0E0E0;
            font-family: "Segoe UI", "Roboto", sans-serif;
            font-size: 10pt;
        }
        QPushButton {
            background-color: #1F1F1F;
            border: 1px solid #3A3A3A;
            padding: 10px 18px;
            border-radius: 8px;
            color: #EEE;
        }
        QPushButton:hover {
            background-color: #2A2A2A;
            border-color: #575757;
        }
        QPushButton:pressed {
            background-color: #111;
        }
        QLineEdit, QComboBox {
            background-color: #1D1D1D;
            border: 1px solid #3A3A3A;
            padding: 8px;
            border-radius: 8px;
            color: #EEE;
        }
        QLabel#titleLabel {
            font-size: 20pt;
            font-weight: bold;
            color: #FFFFFF;
        }
        QLabel#subtitleLabel {
            font-size: 10pt;
            color: #A8A8A8;
        }
        QFrame#card {
            background-color: #181818;
            border: 1px solid #2E2E2E;
            border-radius: 16px;
        }
    )";

    app.setStyleSheet(styleSheet);

    MainWindow window;
    window.resize(420, 440);
    window.show();

    return app.exec();
}
