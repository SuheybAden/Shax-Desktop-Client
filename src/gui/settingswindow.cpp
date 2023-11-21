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
    qDebug() << "Writing settings";
    settings->setValue("mode", ui->modeComboBox->currentText());
    settings->setValue("url", ui->urlLineEdit->text());
}

void SettingsWindow::readSettings(){
    qDebug() << "Reading settings";
    ui->modeComboBox->setCurrentText(settings->value("mode", "Local").toString());
    ui->urlLineEdit->setText(settings->value("url", "ws://192.168.0.21:8765").toString());
}
