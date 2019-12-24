#include "eutils.h"

EUtils::EUtils()
{
}

QString EUtils::formatValue(int value,int frmt, bool is16Bit, bool isSigned=false)
{
    QString convertedValue;

    switch(frmt){

        case 2://Binary
        if (is16Bit)
            if (isSigned)
                convertedValue = QString("%1").arg((signed short)value,16,2,QLatin1Char('0'));
            else
                convertedValue = QString("%1").arg((unsigned short)value,16,2,QLatin1Char('0'));
        else

            convertedValue = QString("%1").arg(value,0,2);
        break;

        case 10://Decimal - Unsigned Integer
            if (isSigned)
                convertedValue = QString("%1").arg((signed short)value,0,10);
            else
                convertedValue = QString("%1").arg((unsigned short)value,0,10);
        break;

        case 16://Hex
            if (is16Bit)
                convertedValue = QString("%1").arg(value,4,16,QLatin1Char('0'));
            else
                convertedValue = QString("%1").arg(value,0,16);
        break;

        default://Default
        convertedValue = QString("%1").arg(value,0,10);

    }

    return convertedValue.toUpper();

}


QString EUtils::libmodbus_strerror(int errnum)
{
    switch (errnum) {

        case EINVAL:
            return "Protocol context is NULL";
            break;

        case ETIMEDOUT:
            return "Timeout";
            break;

        case ECONNRESET:
            return "Connection reset";
            break;

        case ECONNREFUSED:
            return "Connection refused";
            break;

        case EPIPE:
            return "Socket error";
            break;

        default://Default
            return modbus_strerror(errno);

    }

}

