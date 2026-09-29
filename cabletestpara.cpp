#include "cabletestpara.h"
#include "QDebug"
#include <QCoreApplication>

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

QString CableTestPara::shieldType() const
{
    return m_shieldType;
}

void CableTestPara::setShieldType(const QString &newShieldType)
{
    if (m_shieldType == newShieldType)
        return;
    m_shieldType = newShieldType;
    emit shieldTypeChanged();
    qDebug() << "C++" << "内屏蔽类型:" << newShieldType;
}

QString CableTestPara::imagesPath() const
{
    auto findImages = [](const QString &base) -> QString {
        QDir dir(base);
        // 从 base 向上逐级查找 images/ 目录
        for (int i = 0; i < 4; ++i) {
            QString candidate = dir.absolutePath() + "/images/";
            if (QDir(candidate).exists())
                return "file:///" + candidate;
            if (!dir.cdUp())
                break;
        }
        return QString();
    };

    // 1. exe 同目录 → 向上一级级查找（适配所有部署/开发目录结构）
    QString appDir = QCoreApplication::applicationDirPath();
    QString found = findImages(appDir);
    if (!found.isEmpty()) {
        qDebug() << "C++ images 路径:" << found;
        return found;
    }

    // 2. 当前工作目录
    QString cwd = QDir::currentPath();
    found = findImages(cwd);
    if (!found.isEmpty()) {
        qDebug() << "C++ images 路径(工作目录):" << found;
        return found;
    }

    // 兜底
    qDebug() << "C++ 警告：images 文件夹未找到，请确认 images/ 已放置在 exe 同级目录";
    return "file:///" + appDir + "/images/";
}
