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
    settings->setValue("lobby_key", ui->lobbyKeySpinBox->value());
}

void SettingsWindow::readSettings(){
    qDebug() << "Reading settings";
    ui->modeComboBox->setCurrentText(settings->value("mode", "Local").toString());
    ui->urlLineEdit->setText(settings->value("url", "ws://localhost:8765").toString());
    ui->lobbyKeySpinBox->setValue(settings->value("lobby_key", 0).toInt());
}
