#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include <QSettings>

class SettingsWindow : public QDialog
{
    Q_OBJECT
public:
    SettingsWindow(QSettings *settings, QWidget *parent = nullptr);

signals:

};

#endif // SETTINGSWINDOW_H
