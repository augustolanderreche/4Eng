#ifndef USERWINDOW_H
#define USERWINDOW_H

#include <QMainWindow>

class QLabel;

class UserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit UserWindow(const QString &displayName, QWidget *parent = nullptr);

private:
    QLabel *m_welcomeLabel;
};

#endif