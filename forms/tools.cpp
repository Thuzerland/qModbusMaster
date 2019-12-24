#include "tools.h"
#include "ui_tools.h"

#include "QsLog.h"

Tools::Tools(QWidget *parent, ModbusAdapter *adapter, ModbusCommSettings *settings) :
    QMainWindow(parent),
    m_modbusAdapter(adapter), m_modbusCommSettings(settings),
    ui(new Ui::Tools)
{
    //setup UI
    ui->setupUi(this);
    cmbModbusMode = new QComboBox(this);
    cmbModbusMode->setMinimumWidth(96);
    cmbModbusMode->addItem("RTU/TCP");cmbModbusMode->addItem("TCP");
    cmbCmd = new QComboBox(this);
    cmbCmd->setMinimumWidth(96);
    cmbCmd->addItem("Report Slave ID");
    ui->toolBar->addWidget(cmbModbusMode);
    ui->toolBar->addWidget(cmbCmd);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionExec);
    ui->toolBar->addAction(ui->actionClear);
    ui->toolBar->addAction(ui->actionExit);

    //UI - connections
    connect(cmbModbusMode,SIGNAL(currentIndexChanged(int)),this,SLOT(changedModbusMode(int)));
    connect(ui->actionExec,SIGNAL(triggered(bool)),this,SLOT(execCmd()));
    connect(ui->actionClear,SIGNAL(triggered(bool)),this,SLOT(clear()));
    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(exit()));
    connect(&m_pingProc,SIGNAL(readyReadStandardOutput()),this,SLOT(pingData()));
    connect(&m_pingProc,SIGNAL(readyReadStandardError()),this,SLOT(pingData()));

}

Tools::~Tools()
{
    delete ui;
}

void Tools::exit()
{

   this->close();

}

QString Tools::ipConv(QString ip)
{
    /* convert ip - remove leding 0's */

    QStringList m_ip;
    QStringList m_ip_conv;
    QString m_ip_byte;

    m_ip = ip.split(".");
    for (int i = 0; i < m_ip.size(); i++){
         m_ip_byte = m_ip.at(i);
         if (m_ip_byte.at(0)=='0')
             m_ip_conv << m_ip_byte.remove(0,1);
         else
             m_ip_conv << m_ip_byte;
    }

    return  (m_ip_conv.at(0) + "." + m_ip_conv.at(1) + "." + m_ip_conv.at(2) + "." + m_ip_conv.at(3));

}

void Tools::changedModbusMode(int currIndex)
{

    QLOG_TRACE()<<  "Modbus Mode changed. Index = " << currIndex;

    cmbCmd->clear();
    if (currIndex == 0) { //RTU/TCP
       cmbCmd->addItem("Report Slave ID");
    }
    else { //TCP
       cmbCmd->addItem("Report Slave ID");cmbCmd->addItem("Ping");cmbCmd->addItem("Port Status");
    }

}

void Tools::execCmd()
{

    ui->txtOutput->moveCursor(QTextCursor::End);

    QLOG_TRACE()<<  "Tools Execute Cmd " << cmbCmd->currentText();
    switch (cmbCmd->currentIndex()){
        case 0:
        ui->txtOutput->appendPlainText(QString("------- Modbus Diagnotics : Report Slave ID %1 -------\n").arg(m_modbusCommSettings->slaveID()));
        diagnosticsProc();
        break;

        case 1:
        ui->txtOutput->appendPlainText(QString("------- Modbus TCP : Ping IP %1 -------\n").arg(m_modbusCommSettings->slaveIP()));
        pingProc();
        break;

        case 2:
        ui->txtOutput->appendPlainText(QString("------- Modbus TCP : Check Port %1:%2 Status -------\n").arg(m_modbusCommSettings->slaveIP(),m_modbusCommSettings->TCPPort()));
        portProc();
        break;

        default:
        ui->txtOutput->appendPlainText("------- No Valid Selection -------\n");
        break;

    }

}

void Tools::clear()
{

    QLOG_TRACE()<<  "Tools Clear Ouput";
    ui->txtOutput->clear();

}

void Tools::diagnosticsProc()
{

    qApp->processEvents();
    ui->txtOutput->moveCursor(QTextCursor::End);
    if(m_modbusAdapter->m_modbus != NULL){
        modbusDiagnostics();
    }
    else{
        ui->txtOutput->insertPlainText("Not Connected.\n");
    }

}

void Tools::pingProc()
{

    qApp->processEvents();
    m_pingProc.start("ping", QStringList() << ipConv(m_modbusCommSettings->slaveIP()));
    if (m_pingProc.waitForFinished(5000)){
            //just wait -> execute button is pressed
    }
}

void Tools::pingData()
{

    qApp->processEvents();
    ui->txtOutput->moveCursor(QTextCursor::End);
    ui->txtOutput->insertPlainText(m_pingProc.readAll());

}

void Tools::portProc()
{

    qApp->processEvents();
    ui->txtOutput->moveCursor(QTextCursor::End);
    m_portProc.connectToHost(ipConv(m_modbusCommSettings->slaveIP()),m_modbusCommSettings->TCPPort().toInt());
    if (m_portProc.waitForConnected(5000)){//wait -> execute button is pressed
        ui->txtOutput->insertPlainText("Connected.Port is opened\n");
        m_portProc.close();
    }
    else{
         ui->txtOutput->insertPlainText("Not connected.Port is closed\n");
    }

}

void Tools::modbusDiagnostics()
{
    //Modbus diagnostics - RTU/TCP
    QLOG_TRACE()<<  "Modbus diagnostics.";

    //Modbus data
    m_modbusAdapter->setFunctionCode(0x11);
    uint8_t dest[1024]; //setup memory for data
    memset(dest, 0, 1024);
    int ret = -1; //return value from read functions

    modbus_set_slave(m_modbusAdapter->m_modbus, m_modbusCommSettings->slaveID());
    //request data from modbus
    ret = modbus_report_slave_id(m_modbusAdapter->m_modbus, MODBUS_MAX_PDU_LENGTH, dest);
    QLOG_TRACE() <<  "Modbus Read Data return value = " << ret << ", errno = " << errno;

    //update data model
    if(ret > 1)
    {
        QString line;
        line = dest[1]?"ON":"OFF";
        ui->txtOutput->insertPlainText("Run Status : " + line + "\n");
        QString id = QString::fromUtf8((char*)dest);
        ui->txtOutput->insertPlainText("ID : " + id.right(id.size()-2) + "\n");;
    }
    else
    {
        QString line = "";
        if(ret < 0) {
                line = QString("Error : ") +  EUtils::libmodbus_strerror(errno);
                QLOG_ERROR() <<  "Read diagnostics data failed. " << line;
                line = QString(tr("Read diagnostics data failed.\nError : ")) +  EUtils::libmodbus_strerror(errno);
                ui->txtOutput->insertPlainText(line);
        }
        else {
                line = QString("Unknown Error : ")  +  EUtils::libmodbus_strerror(errno);
                QLOG_ERROR() <<  "Read diagnostics data failed. " << line;
                line = QString(tr("Read diagnostics data failed.\nUnknown Error : "))  +  EUtils::libmodbus_strerror(errno);
                ui->txtOutput->insertPlainText(line);
        }

        modbus_flush(m_modbusAdapter->m_modbus); //flush data
     }

}

