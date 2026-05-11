#include "userwindow.h"
#include <QLabel>
#include <QVBoxLayout>

UserWindow::UserWindow(const QString &displayName, QWidget *parent)
    : QMainWindow(parent)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    m_welcomeLabel = new QLabel(tr("Hola Usuario %1").arg(displayName), central);
    m_welcomeLabel->setStyleSheet("font-size: 24pt; font-weight: bold; color: #FFFFFF;");
    m_welcomeLabel->setAlignment(Qt::AlignCenter);

    auto *layout = new QVBoxLayout(central);
    layout->addStretch();
    layout->addWidget(m_welcomeLabel);
    layout->addStretch();
    layout->setContentsMargins(24, 24, 24, 24);

    setWindowTitle(tr("Panel usuario"));
    resize(500, 280);
}
