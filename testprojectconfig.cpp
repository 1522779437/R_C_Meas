#include "testprojectconfig.h"
#include "QDebug"

TestProjectConfig::TestProjectConfig(QObject *parent)
    : QObject{parent}
{}

bool TestProjectConfig::checkDC() const
{
    return m_checkDC;
}

void TestProjectConfig::setCheckDC(bool newCheckDC)
{
    if (m_checkDC == newCheckDC)
        return;
    m_checkDC = newCheckDC;
    emit checkDCChanged();
    qDebug() << "C++" << "setCheckDC = " << newCheckDC;
}

bool TestProjectConfig::checkIns() const
{
    return m_checkIns;
}

void TestProjectConfig::setCheckIns(bool newCheckIns)
{
    if (m_checkIns == newCheckIns)
        return;
    m_checkIns = newCheckIns;
    emit checkInsChanged();
    qDebug() << "C++" << "setCheckIns = " << newCheckIns;

}

bool TestProjectConfig::checkCap() const
{
    return m_checkCap;
}

void TestProjectConfig::setCheckCap(bool newCheckCap)
{
    if (m_checkCap == newCheckCap)
        return;
    m_checkCap = newCheckCap;
    emit checkCapChanged();
    qDebug() << "C++" << "setCheckCap = " << newCheckCap;

}
