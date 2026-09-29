#include "cabletestpara.h"
#include "QDebug"
CableTestPara::CableTestPara(QObject *parent)
    : QObject{parent}
{}

QString CableTestPara::selectedCableType() const
{
    return m_selectedCableType;
}

void CableTestPara::setSelectedCableType(const QString &newSelectedCableType)
{
    if (m_selectedCableType == newSelectedCableType)
        return;
    m_selectedCableType = newSelectedCableType;
    emit selectedCableTypeChanged();
    qDebug() << "C++" << "电缆类型:" << newSelectedCableType;
}

int CableTestPara::cableCoreCount() const
{
    return m_cableCoreCount;
}

void CableTestPara::setCableCoreCount(int newCableCoreCount)
{
    if (m_cableCoreCount == newCableCoreCount)
        return;
    m_cableCoreCount = newCableCoreCount;
    emit cableCoreCountChanged();
    qDebug() << "C++" << "电缆芯数:" << newCableCoreCount;
}

QString CableTestPara::unit() const
{
    return m_unit;
}

void CableTestPara::setUnit(const QString &newUnit)
{
    if (m_unit == newUnit)
        return;
    m_unit = newUnit;
    emit unitChanged();
    qDebug() << "C++" << "电缆长度单位:" << newUnit;
}

float CableTestPara::cableLen() const
{
    return m_cableLen;
}

void CableTestPara::setCableLen(float newCableLen)
{
    if (m_cableLen == newCableLen)
        return;
    m_cableLen = newCableLen;
    emit cableLenChanged();
    qDebug() << "C++" << "电缆长度:" << newCableLen;

}
