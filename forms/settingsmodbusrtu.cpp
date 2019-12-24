#include <QtDebug>
#include "settingsmodbusrtu.h"
#include "ui_settingsmodbusrtu.h"

SettingsModbusRTU::SettingsModbusRTU(QWidget *parent,ModbusCommSettings * settings) :
    QDialog(parent),
    ui(new Ui::SettingsModbusRTU),
    m_settings(settings)
{
    ui->setupUi(this);

    /* device name is needed only in Linux */
    #ifdef Q_OS_WIN32
        ui->cmbDev->setDisabled(true);
    #else
        ui->cmbDev->setDisabled(false);
    #endif

    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(changesAccepted()));

}

SettingsModbusRTU::~SettingsModbusRTU()
{
    delete ui;
}

void SettingsModbusRTU::showEvent(QShowEvent * event)
{

    //Load Settings
    if (m_settings != NULL) {

         ui->cmbRTS->clear();

        //Populate cmbPort-cmbRTS
        #ifdef Q_OS_WIN32
            ui->cmbRTS->addItem("Disable");
            ui->cmbRTS->addItem("Enable");
            ui->cmbRTS->addItem("HandShake");
            ui->cmbRTS->addItem("Toggle");
        #else
            ui->cmbRTS->addItem("None");
            ui->cmbRTS->addItem("Up");
            ui->cmbRTS->addItem("Down");
        #endif

        ui->cmbDev->setCurrentText(m_settings->serialDev());
        ui->sbPort->setValue(m_settings->serialPort().toInt());
        ui->cmbBaud->setCurrentIndex(ui->cmbBaud->findText(m_settings->baud()));
        ui->cmbDataBits->setCurrentIndex(ui->cmbDataBits->findText(m_settings->dataBits()));
        ui->cmbStopBits->setCurrentIndex(ui->cmbStopBits->findText(m_settings->stopBits()));
        ui->cmbParity->setCurrentIndex(ui->cmbParity->findText(m_settings->parity()));
        ui->cmbRTS->setCurrentIndex(ui->cmbRTS->findText(m_settings->RTS()));
    }


}

void SettingsModbusRTU::changesAccepted()
{

    //Save Settings
    if (m_settings != NULL) {

        m_settings->setSerialPort(QString::number(ui->sbPort->value()), ui->cmbDev->currentText());
        m_settings->setBaud(ui->cmbBaud->currentText());
        m_settings->setDataBits(ui->cmbDataBits->currentText());
        m_settings->setStopBits(ui->cmbStopBits->currentText());
        m_settings->setParity(ui->cmbParity->currentText());
        m_settings->setRTS((QString)ui->cmbRTS->currentText());
    }

}

