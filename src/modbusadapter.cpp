#include <QApplication>
#include <QtDebug>
#include "modbusadapter.h"
#include "mainwindow.h"

#include "QsLog.h"
#include <errno.h>

ModbusAdapter *m_instance;

ModbusAdapter::ModbusAdapter(QObject *parent) :
    QObject(parent),
    m_modbus(NULL)
{
    m_instance = this;
    regModel=new RegistersModel(this);
    rawModel=new RawDataModel(this);
    m_connected = false;
    m_ModBusMode = EUtils::None;
    m_pollTimer = new QTimer(this);
    m_timeOut = 0;
    m_transactionIsPending = false;
    m_packets = 0;
    m_errors = 0;
    connect(m_pollTimer,SIGNAL(timeout()),this,SLOT(modbusTransaction()));
    connect(regModel,SIGNAL(refreshView()),this,SIGNAL(refreshView()));
    //setup memory for data
    dest = (uint8_t *) malloc(2000 * sizeof(uint8_t));
    memset(dest, 0, 2000 * sizeof(uint8_t));
    dest16 = (uint16_t *) malloc(125 * sizeof(uint16_t));
    memset(dest16, 0, 125 * sizeof(uint16_t));
}

ModbusAdapter::~ModbusAdapter()
{
    free(dest);
    free(dest16);
}

void ModbusAdapter::modbusConnectRTU(QString port, int baud, QChar parity, int dataBits, int stopBits, int RTS, int timeOut)
{
    //Modbus RTU connect
    QString line;
    modbusDisConnect();

    QLOG_INFO()<<  "Modbus Connect RTU";

    m_modbus = modbus_new_rtu(port.toLatin1().constData(),baud,parity.toLatin1(),dataBits,stopBits,RTS);
    line = "Connecting to Serial Port [" + port + "]...";
    QLOG_TRACE() <<  line;

    //Debug messages from libmodbus
    #ifdef LIB_MODBUS_DEBUG_OUTPUT
        modbus_set_debug(m_modbus, 1);
    #endif

    m_timeOut = timeOut;

    if(m_modbus == NULL){
        mainWin->showUpInfoBar(tr("Unable to create the libmodbus context."), InfoBar::Error);
        QLOG_ERROR()<<  "Connection failed. Unable to create the libmodbus context";
        return;
    }
    else if(m_modbus && modbus_set_slave(m_modbus, m_slave) == -1){
        modbus_free(m_modbus);
        mainWin->showUpInfoBar(tr("Invalid slave ID."), InfoBar::Error);
        QLOG_ERROR()<<  "Connection failed. Invalid slave ID";
        return;
    }
    else if(m_modbus && modbus_connect(m_modbus) == -1) {
        modbus_free(m_modbus);
        mainWin->showUpInfoBar(tr("Connection failed\nCould not connect to serial port."), InfoBar::Error);
        QLOG_ERROR()<<  "Connection failed. Could not connect to serial port";
        m_connected = false;
        line += "Failed";
    }
    else {
        //error recovery mode
        modbus_set_error_recovery(m_modbus, MODBUS_ERROR_RECOVERY_PROTOCOL);
        //response_timeout;
        modbus_set_response_timeout(m_modbus, timeOut, 0);
        m_connected = true;
        line += "OK";
        mainWin->hideInfoBar();
        QLOG_TRACE() << line;
    }

    m_ModBusMode = EUtils::RTU;

    //Add line to raw data model
    line = EUtils::SysTimeStamp() + " - " + line;
    rawModel->addLine(line);

}

void ModbusAdapter::modbusConnectTCP(QString ip, int port, int timeOut)
{
    //Modbus TCP connect
    QString strippedIP = "";
    QString line;
    modbusDisConnect();

    QLOG_INFO()<<  "Modbus Connect TCP";

    line = "Connecting to IP : " + ip + ":" + QString::number(port);
    QLOG_TRACE() <<  line;
    strippedIP = stripIP(ip);
    if (strippedIP == ""){
        mainWin->showUpInfoBar(tr("Connection failed\nBlank IP Address."), InfoBar::Error);
        QLOG_ERROR()<<  "Connection failed. Blank IP Address";
        return;
    }
    else {
        m_modbus = modbus_new_tcp(strippedIP.toLatin1().constData(), port);
        mainWin->hideInfoBar();
        QLOG_TRACE() <<  "Connecting to IP : " << ip << ":" << port;
    }

    //Debug messages from libmodbus
    #ifdef LIB_MODBUS_DEBUG_OUTPUT
        modbus_set_debug(m_modbus, 1);
    #endif

    m_timeOut = timeOut;

    if(m_modbus == NULL){
        mainWin->showUpInfoBar(tr("Unable to create the libmodbus context."), InfoBar::Error);
        QLOG_ERROR()<<  "Connection failed. Unable to create the libmodbus context";
        return;
    }
    else if(m_modbus && modbus_connect(m_modbus) == -1) {
        modbus_free(m_modbus);
        mainWin->showUpInfoBar(tr("Connection failed\nCould not connect to TCP port."), InfoBar::Error);
        QLOG_ERROR()<<  "Connection to IP : " << ip << ":" << port << "...failed. Could not connect to TCP port";
        m_connected = false;
        line += " Failed";
    }
    else {
        //error recovery mode
        modbus_set_error_recovery(m_modbus, MODBUS_ERROR_RECOVERY_PROTOCOL);
        //response_timeout;
        modbus_set_response_timeout(m_modbus, timeOut, 0);
        m_connected = true;
        line += " OK";
        mainWin->hideInfoBar();
        QLOG_TRACE() << line;
    }

    m_ModBusMode = EUtils::TCP;

    //Add line to raw data model
    line = EUtils::SysTimeStamp() + " - " + line;
    rawModel->addLine(line);

}


void ModbusAdapter::modbusDisConnect()
{
    //Modbus disconnect

    QLOG_INFO()<<  "Modbus disconnected";

    if(m_modbus) {
        if (m_connected){
            modbus_close(m_modbus);
            modbus_free(m_modbus);
        }
        m_modbus = NULL;
    }

    m_connected = false;

    m_ModBusMode = EUtils::None;

}

bool ModbusAdapter::isConnected()
{
    //Modbus is connected

    return m_connected;
}

void ModbusAdapter::modbusTransaction()
{
    //Modbus request data

    QLOG_INFO() <<  "Modbus Transaction. Function Code = " << m_functionCode;
    m_packets += 1;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    switch(m_functionCode)
    {
            case MODBUS_FC_READ_COILS:
            case MODBUS_FC_READ_DISCRETE_INPUTS:
            case MODBUS_FC_READ_HOLDING_REGISTERS:
            case MODBUS_FC_READ_INPUT_REGISTERS:
                    modbusReadData(m_slave,m_functionCode,m_startAddr,m_numOfRegs);
                    break;

            case MODBUS_FC_WRITE_SINGLE_COIL:
            case MODBUS_FC_WRITE_SINGLE_REGISTER:
            case MODBUS_FC_WRITE_MULTIPLE_COILS:
            case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
                    modbusWriteData(m_slave,m_functionCode,m_startAddr,m_numOfRegs);
                    break;
            default:
                    break;
    }

    QApplication::setOverrideCursor(Qt::ArrowCursor);

    emit(refreshView());

}

void ModbusAdapter::modbusReadData(int slave, int functionCode, int startAddress, int noOfItems)
{

    QLOG_INFO() <<  "Modbus Read Data ";

    if(m_modbus == NULL) return;

    int ret = -1; //return value from read functions
    bool is16Bit = false;

    modbus_set_slave(m_modbus, slave);
    //request data from modbus
    switch(functionCode)
    {
            case MODBUS_FC_READ_COILS:
                    ret = modbus_read_bits(m_modbus, startAddress, noOfItems, dest);
                    break;

            case MODBUS_FC_READ_DISCRETE_INPUTS:
                    ret = modbus_read_input_bits(m_modbus, startAddress, noOfItems, dest);
                    break;

            case MODBUS_FC_READ_HOLDING_REGISTERS:
                    ret = modbus_read_registers(m_modbus, startAddress, noOfItems, dest16);
                    is16Bit = true;
                    break;

            case MODBUS_FC_READ_INPUT_REGISTERS:
                    ret = modbus_read_input_registers(m_modbus, startAddress, noOfItems, dest16);
                    is16Bit = true;
                    break;

            default:
                    break;
    }

    QLOG_TRACE() <<  "Modbus Read Data return value = " << ret << ", errno = " << errno;

    //update data model
    if(ret == noOfItems)
    {
            for(int i = 0; i < noOfItems; ++i)
            {
                int data = is16Bit ? dest16[i] : dest[i];
                regModel->setValue(i,data);
            }
            mainWin->hideInfoBar();
    }
    else
    {

        regModel->setNoValidValues();
        m_errors += 1;

        QString line = "";
        if(ret < 0) {
                line = QString("Error : ") +  EUtils::libmodbus_strerror(errno);
                QLOG_ERROR() <<  "Read Data failed. " << line;
                rawModel->addLine(EUtils::SysTimeStamp() + " - " + line);
                line = QString(tr("Read data failed.\nError : ")) +  EUtils::libmodbus_strerror(errno);
        }
        else {
                line = QString("Number of registers returned does not match number of registers requested!. Error : ")  +  EUtils::libmodbus_strerror(errno);
                QLOG_ERROR() <<  "Read Data failed. " << line;
                rawModel->addLine(EUtils::SysTimeStamp() + " - " + line);
                line = QString(tr("Read data failed.\nNumber of registers returned does not match number of registers requested!. Error : "))  +  EUtils::libmodbus_strerror(errno);
        }

        mainWin->showUpInfoBar(line, InfoBar::Error);
        modbus_flush(m_modbus); //flush data
     }

}

void ModbusAdapter::modbusWriteData(int slave, int functionCode, int startAddress, int noOfItems)
{

    QLOG_INFO() <<  "Modbus Write Data ";

    if(m_modbus == NULL) return;

    int ret = -1; //return value from functions

    modbus_set_slave(m_modbus, slave);
    //request data from modbus
    switch(functionCode)
    {
            case MODBUS_FC_WRITE_SINGLE_COIL:
                    ret = modbus_write_bit(m_modbus, startAddress,regModel->value(0));
                    noOfItems = 1;
                    break;

            case MODBUS_FC_WRITE_SINGLE_REGISTER:
                    ret = modbus_write_register( m_modbus, startAddress,regModel->value(0));
                    noOfItems = 1;
                    break;

            case MODBUS_FC_WRITE_MULTIPLE_COILS:
            {
                    uint8_t * data = new uint8_t[noOfItems];
                    for(int i = 0; i < noOfItems; ++i)
                    {
                            data[i] = regModel->value(i);
                    }
                    ret = modbus_write_bits(m_modbus, startAddress, noOfItems, data);
                    delete[] data;
                    break;
            }
            case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            {
                    uint16_t * data = new uint16_t[noOfItems];
                    for(int i = 0; i < noOfItems; ++i)
                    {
                            data[i] = regModel->value(i);
                    }
                    ret = modbus_write_registers(m_modbus, startAddress, noOfItems, data);
                    delete[] data;
                    break;
            }

            default:
                    break;
    }

    QLOG_TRACE() <<  "Modbus Write Data return value = " << ret << ", errno = " << errno;;

    //update data model
    if(ret == noOfItems)
    {
        //values written correctly
        rawModel->addLine(EUtils::SysTimeStamp() + " - values written correctly.");
        mainWin->hideInfoBar();
    }
    else
    {

        regModel->setNoValidValues();
        m_errors += 1;

        QString line;
        if(ret < 0) {
                line = QString("Error : ") +  EUtils::libmodbus_strerror(errno);
                QLOG_ERROR() <<  "Write Data failed. " << line;
                rawModel->addLine(EUtils::SysTimeStamp() + " - " + line);
                line = QString(tr("Write data failed.\nError : ")) +  EUtils::libmodbus_strerror(errno);
        }
        else {
                line = QString("Number of registers returned does not match number of registers requested!. Error : ")  +  EUtils::libmodbus_strerror(errno);
                QLOG_ERROR() <<  "Write Data failed. " << line;
                rawModel->addLine(EUtils::SysTimeStamp() + " - " + line);
                line = QString(tr("Write data failed.\nNumber of registers returned does not match number of registers requested!. Error : "))  +  EUtils::libmodbus_strerror(errno);
         }

        mainWin->showUpInfoBar(line, InfoBar::Error);
        modbus_flush(m_modbus); //flush data
     }

}

void ModbusAdapter::busMonitorRequestData(uint8_t * data, int dataLen)
{

    //Request Raw data from port - Update raw data model

    QString line;

    for(int i = 0; i < dataLen; ++i ) {
        line += QString().sprintf( "%.2x  ", data[i] );
    }

    QLOG_INFO() << "Tx Data : " << line;
    line = EUtils::TxTimeStamp(m_ModBusMode) + " - " + line.toUpper();

    rawModel->addLine(line);

    m_transactionIsPending = true;

}
void ModbusAdapter::busMonitorResponseData(uint8_t * data, int dataLen)
{

    //Response Raw data from port - Update raw data model

    QString line;

    for(int i = 0; i < dataLen; ++i ) {
        line += QString().sprintf( "%.2x  ", data[i] );
    }

    QLOG_INFO() << "Rx Data : " << line;
    line = EUtils::RxTimeStamp(m_ModBusMode) + " - " + line.toUpper();

    rawModel->addLine(line);

    m_transactionIsPending = false;

}

void ModbusAdapter::setSlave(int slave)
{
    m_slave = slave;
}

void ModbusAdapter::setFunctionCode(int functionCode)
{
    m_functionCode = functionCode;
}

void ModbusAdapter::setStartAddr(int addr)
{
    m_startAddr = addr;
}

void ModbusAdapter::setNumOfRegs(int num)
{
    m_numOfRegs = num;
}

void ModbusAdapter::addItems()
{
    regModel->addItems(m_startAddr, m_numOfRegs, EUtils::ModbusIsWriteFunction(m_functionCode));
    //If it is a write function -> read registers
    if (!m_connected)
        return;
    else if (EUtils::ModbusIsWriteCoilsFunction(m_functionCode)){
        modbusReadData(m_slave,EUtils::ReadCoils,m_startAddr,m_numOfRegs);
        emit(refreshView());
    }
    else if (EUtils::ModbusIsWriteRegistersFunction(m_functionCode)){
        modbusReadData(m_slave,EUtils::ReadHoldRegs,m_startAddr,m_numOfRegs);
        emit(refreshView());
    }

}

void ModbusAdapter::setScanRate(int scanRate)
{
    m_scanRate = scanRate;
}

void ModbusAdapter::resetCounters()
{
    m_packets = 0;
    m_errors = 0;
    emit(refreshView());
}

int ModbusAdapter::packets()
{
    return m_packets;
}

int ModbusAdapter::errors()
{
    return m_errors;
}

void ModbusAdapter::startPollTimer()
{
    m_pollTimer->start(m_scanRate);
}

void ModbusAdapter::stopPollTimer()
{
    m_pollTimer->stop();
}

void ModbusAdapter::setTimeOut(int timeOut)
{

    m_timeOut = timeOut;

}

QString ModbusAdapter::stripIP(QString ip)
{
    //Strip zero's from IP
    QStringList ipBytes;
    QString res = "";
    int i;

    ipBytes = ip.split(".");
    if (ipBytes.size() == 4){
        res = QString("").setNum(ipBytes[0].toInt());
        i = 1;
        while (i <  ipBytes.size()){
            res = res + "." + QString("").setNum(ipBytes[i].toInt());
            i++;
        }
        return res;
    }
    else
        return "";

}

extern "C" {

void busMonitorRawResponseData(uint8_t * data, int dataLen)
{
        m_instance->busMonitorResponseData(data, dataLen);
}

void busMonitorRawRequestData(uint8_t * data, int dataLen)
{
        m_instance->busMonitorRequestData(data, dataLen);
}

}
