#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>

class QLabel;
class QTabWidget;

class AdminWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminWindow(const QString &displayName, QWidget *parent = nullptr);

private:
    void setupUi(const QString &displayName);
    QWidget *createDashboardTab();
    QWidget *createUsersTab();
    QWidget *createCompaniesTab();
    QWidget *createAcceptedTab();
    QWidget *createIaTab();

    QLabel *m_welcomeLabel;
    QTabWidget *m_tabs;
};

#endif // ADMINWINDOW_H
