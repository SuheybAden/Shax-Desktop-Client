#include "settingswindow.h"
#include "./ui_settingswindow.h"

SettingsWindow::SettingsWindow(QSettings *settings, QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    connectSignals();

    this->settings = settings;
    readSettings();
}

void SettingsWindow::connectSignals(){
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsWindow::writeSettings);
}

void SettingsWindow::writeSettings(){
    settings->setValue("url", ui->urlLineEdit->text());
}

void SettingsWindow::readSettings(){
    ui->urlLineEdit->setText(settings->value("url", "ws://localhost:8765").toString());
}
