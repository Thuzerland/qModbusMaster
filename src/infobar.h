#ifndef _MyInfoBar_H_
#define _MyInfoBar_H_

#include <QLabel>
#include <QFrame>

class InfoBar : public QFrame
{
    Q_OBJECT
    private:
        QLabel *label;
    public:
        enum InfoType {
            Question,
            Warning,
            Error
        };

        InfoBar(QWidget *parent = 0);
        InfoBar(QString message, InfoType type, QWidget *parent = 0);
        void setInfoType(InfoType type);
        void setMessage(QString message);
        void show(QString message, InfoType type);
};

#endif
