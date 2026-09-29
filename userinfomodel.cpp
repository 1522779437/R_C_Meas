#include "userinfomodel.h"
#include "QDebug"

UserInfoModel::UserInfoModel(QObject *parent)
    : QObject{parent}
{

}

QString UserInfoModel::outId() const
{
    return m_outId;
}

void UserInfoModel::setOutId(const QString &newOutId)
{
    if (m_outId == newOutId)
        return;
    m_outId = newOutId;
    emit outIdChanged();
    qDebug() << "C++" << "m_outId = " << m_outId;
}

QString UserInfoModel::spec() const
{
    return m_spec;
}

void UserInfoModel::setSpec(const QString &newSpec)
{
    if (m_spec == newSpec)
        return;
    m_spec = newSpec;
    emit specChanged();
    qDebug() << "C++" << "newSpec = " << newSpec;

}

QString UserInfoModel::outerEnd() const
{
    return m_outerEnd;
}

void UserInfoModel::setOuterEnd(const QString &newOuterEnd)
{
    if (m_outerEnd == newOuterEnd)
        return;
    m_outerEnd = newOuterEnd;
    emit outerEndChanged();
    qDebug() << "C++" << "newOuterEnd = " << newOuterEnd;

}

QString UserInfoModel::length() const
{
    return m_length;
}

void UserInfoModel::setLength(const QString &newLength)
{
    if (m_length == newLength)
        return;
    m_length = newLength;
    emit lengthChanged();
    qDebug() << "C++" << "newLength = " << newLength;

}

QString UserInfoModel::appearance() const
{
    return m_appearance;
}

void UserInfoModel::setAppearance(const QString &newAppearance)
{
    if (m_appearance == newAppearance)
        return;
    m_appearance = newAppearance;
    emit appearanceChanged();
    qDebug() << "C++" << "newAppearance = " << newAppearance;

}


QString UserInfoModel::leak() const
{
    return m_leak;
}

void UserInfoModel::setLeak(const QString &newLeak)
{
    if (m_leak == newLeak)
        return;
    m_leak = newLeak;
    emit leakChanged();
    qDebug() << "C++" << "newLeak = " << newLeak;

}

QString UserInfoModel::serialNum() const
{
    return m_serialNum;
}

void UserInfoModel::setSerialNum(const QString &newSerialNum)
{
    if (m_serialNum == newSerialNum)
        return;
    m_serialNum = newSerialNum;
    emit serialNumChanged();
    qDebug() << "C++" << "newSerialNum = " << newSerialNum;

}

QString UserInfoModel::testDate() const
{
    return m_testDate;
}

void UserInfoModel::setTestDate(const QString &newTestDate)
{
    if (m_testDate == newTestDate)
        return;
    m_testDate = newTestDate;
    emit testDateChanged();
    qDebug() << "C++" << "newTestDate = " << newTestDate;

}

QString UserInfoModel::testEnvironment() const
{
    return m_testEnvironment;
}

void UserInfoModel::setTestEnvironment(const QString &newTestEnvironment)
{
    if (m_testEnvironment == newTestEnvironment)
        return;
    m_testEnvironment = newTestEnvironment;
    emit testEnvironmentChanged();
    qDebug() << "C++" << "newTestEnvironment = " << newTestEnvironment;

}

QString UserInfoModel::testLocation() const
{
    return m_testLocation;
}

void UserInfoModel::setTestLocation(const QString &newTestLocation)
{
    if (m_testLocation == newTestLocation)
        return;
    m_testLocation = newTestLocation;
    emit testLocationChanged();
    qDebug() << "C++" << "newTestLocation = " << newTestLocation;

}

QString UserInfoModel::techDirector() const
{
    return m_techDirector;
}

void UserInfoModel::setTechDirector(const QString &newTechDirector)
{
    if (m_techDirector == newTechDirector)
        return;
    m_techDirector = newTechDirector;
    emit techDirectorChanged();
    qDebug() << "C++" << "newTechDirector = " << newTechDirector;

}

QString UserInfoModel::superUnit() const
{
    return m_superUnit;
}

void UserInfoModel::setSuperUnit(const QString &newSuperUnit)
{
    if (m_superUnit == newSuperUnit)
        return;
    m_superUnit = newSuperUnit;
    emit superUnitChanged();
    qDebug() << "C++" << "newSuperUnit = " << newSuperUnit;

}
