#ifndef EMPRESAWINDOW_H
#define EMPRESAWINDOW_H

#include <QMainWindow>

class QLabel;

class EmpresaWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EmpresaWindow(const QString &displayName, QWidget *parent = nullptr);

private:
    QLabel *m_welcomeLabel;
};

#endif // EMPRESAWINDOW_H