#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QLabel>
#include <QString>
#include <QProcess>

#include "forms/about.h"
#include "forms/settingsmodbusrtu.h"
#include "forms/settingsmodbustcp.h"
#include "forms/settings.h"
#include "forms/busmonitor.h"
#include "forms/tools.h"
#include "modbuscommsettings.h"
#include "modbusadapter.h"
#include "infobar.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, ModbusAdapter *adapter = 0, ModbusCommSettings *settings = 0);
    ~MainWindow();
    void showUpInfoBar(QString message, InfoBar::InfoType type);
    void hideInfoBar();

private:
    Ui::MainWindow *ui;
    //UI - Dialogs
    About *m_dlgAbout;
    SettingsModbusRTU *m_dlgModbusRTU;
    SettingsModbusTCP *m_dlgModbusTCP;
    Settings *m_dlgSettings;
    BusMonitor *m_busMonitor;
    Tools *m_tools;

    ModbusCommSettings *m_modbusCommSettings;
    void updateStatusBar();
    QLabel *m_statusText;
    QLabel *m_statusInd;
    QLabel *m_baseAddr;
    QLabel *m_statusPackets;
    QLabel *m_statusErrors;
    ModbusAdapter *m_modbus;
    void modbusConnect(bool connect);

    void changeEvent(QEvent* event);

private slots:
    void showSettingsModbusRTU();
    void showSettingsModbusTCP();
    void showSettings();
    void showBusMonitor();
    void showTools();
    void changedModbusMode(int currIndex);
    void changedFunctionCode(int currIndex);
    void changedBase(int currIndex);
    void changedDecSign(bool value);
    void changedStartAddrBase(int currIndex);
    void changedScanRate(int value);
    void changedConnect(bool value);
    void changedStartAddress(int value);
    void changedNoOfRegs(int value);
    void changedSlaveID(int value);
    void addItems();
    void clearItems();
    void openLogFile();
    void modbusScanCycle(bool value);
    void modbusRequest();
    void refreshView();
    void changeLanguage();
    void openModbusManual();
    void loadSession();
    void saveSession();
    void showHeaders(bool value);

signals:
    void resetCounters();

};

extern MainWindow *mainWin;

#endif // MAINWINDOW_H
