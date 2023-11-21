#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsWindow; }
QT_END_NAMESPACE

class SettingsWindow : public QDialog
{
    Q_OBJECT
public:
    SettingsWindow(QSettings *settings, QWidget *parent = nullptr);

public slots:
    void writeSettings();

signals:


private:
    Ui::SettingsWindow *ui;

    QSettings *settings;

    void connectSignals();
    void readSettings();
};

#endif // SETTINGSWINDOW_H
