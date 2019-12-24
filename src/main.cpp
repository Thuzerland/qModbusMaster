#include <QApplication>
#include <stdio.h>
#include <stdlib.h>
#include <QDir>
#include <QTranslator>

#include "QsLog.h"
#include "QsLogDest.h"
#include "mainwindow.h"
#include "modbusadapter.h"
#include "modbuscommsettings.h"

QTranslator *Translator;

//Logging Levels
//TraceLevel : 0
//DebugLevel : 1
//InfoLevel : 2
//WarnLevel : 3
//ErrorLevel : 4
//FatalLevel : 5
//OffLevel : 6

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    Translator = new QTranslator;
    Translator->load(":/translations/" + QCoreApplication::applicationName() + "_" + QLocale::system().name());
    app.installTranslator(Translator);

    //init the logging mechanism
    QsLogging::Logger& logger = QsLogging::Logger::instance();
    logger.setLoggingLevel(QsLogging::OffLevel); // start with no logging
    const QString sLogPath(QDir(app.applicationDirPath()).filePath("QModMaster.log"));
    QsLogging::DestinationPtr fileDestination(QsLogging::DestinationFactory::MakeFileDestination(sLogPath,true,65535,2));
    QsLogging::DestinationPtr debugDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());
    logger.addDestination(debugDestination);
    logger.addDestination(fileDestination);

    //Modbus Adapter
    ModbusAdapter modbus_adapt(NULL);
    //Program settings
    ModbusCommSettings settings("qModMaster.ini");

    //show main window
    mainWin = new MainWindow(NULL, &modbus_adapt, &settings);
    //connect signals - slots
    QObject::connect(&modbus_adapt, SIGNAL(refreshView()), mainWin, SLOT(refreshView()));
    QObject::connect(mainWin, SIGNAL(resetCounters()), &modbus_adapt, SLOT(resetCounters()));
    mainWin->show();

    return app.exec();

}
