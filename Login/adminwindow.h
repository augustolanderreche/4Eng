#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>

class QLabel;

class AdminWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminWindow(const QString &displayName, QWidget *parent = nullptr);

private:
    QLabel *m_welcomeLabel;
};

#endif