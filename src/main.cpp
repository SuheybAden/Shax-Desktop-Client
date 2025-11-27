#include "gui/mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QtMessageHandler>

QString logFileName = "log.txt";

QFile logFile(logFileName);
QtMessageHandler originalHandler = nullptr;


void logToFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	// If the log file can't be opened, just use the default logging method
	if(!logFile.open(QIODevice::Append, QFileDevice::WriteUser | QFileDevice::ReadUser)){
		originalHandler(type, context, msg);
		return;
	}

    QTextStream stream(&logFile);

	// Construct the log string
    QString level;
    switch (type) {
		case QtDebugMsg:
			level = "DEBUG";
			break;
		case QtInfoMsg:
			level = "INFO";
			break;
		case QtWarningMsg:
			level = "WARNING";
			break;
		case QtCriticalMsg:
			level = "CRITICAL";
			break;
		case QtFatalMsg:
			level = "FATAL";
			break;
    }

    stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
           << " [" << level << "] "
           << msg << "\n";

	// Flush the log message
    stream.flush();

	// Close the log file
	logFile.close();
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QTranslator translator;
	const QStringList uiLanguages = QLocale::system().uiLanguages();
	for (const QString &locale : uiLanguages) {
		const QString baseName = "shax-desktop-client_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName)) {
			a.installTranslator(&translator);
			break;
		}
	}

	// Delete the previous backup log
	QFile::remove(logFileName + ".bak");

	// Backup the previous log file
	QFile::rename(logFileName, logFileName + ".bak");

	// Attach the new message handler
	// Hold onto the original message handler as a backup option
	originalHandler = qInstallMessageHandler(logToFile);

	MainWindow w;
	QApplication::setStyle("fusion");
	w.show();
	return a.exec();
}
