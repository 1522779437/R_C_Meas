#ifndef TESTPROJECTCONFIG_H
#define TESTPROJECTCONFIG_H

#include <QObject>

class TestProjectConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool checkDC READ checkDC WRITE setCheckDC NOTIFY checkDCChanged FINAL)
    Q_PROPERTY(bool checkIns READ checkIns WRITE setCheckIns NOTIFY checkInsChanged FINAL)
    Q_PROPERTY(bool checkCap READ checkCap WRITE setCheckCap NOTIFY checkCapChanged FINAL)

public:
    explicit TestProjectConfig(QObject *parent = nullptr);

    bool checkDC() const;
    void setCheckDC(bool newCheckDC);

    bool checkIns() const;
    void setCheckIns(bool newCheckIns);

    bool checkCap() const;
    void setCheckCap(bool newCheckCap);

signals:
    void checkDCChanged();
    void checkInsChanged();

    void checkCapChanged();

private:
    bool m_checkDC = false;
    bool m_checkIns = false;
    bool m_checkCap = false;
};

#endif // TESTPROJECTCONFIG_H
