#ifndef CABLETESTPARA_H
#define CABLETESTPARA_H

#include <QObject>
#include <QString>
#include <QDir>

class CableTestPara : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString selectedCableType READ selectedCableType WRITE setSelectedCableType NOTIFY selectedCableTypeChanged FINAL)
    Q_PROPERTY(int cableCoreCount READ cableCoreCount WRITE setCableCoreCount NOTIFY cableCoreCountChanged FINAL)
    Q_PROPERTY(QString unit READ unit WRITE setUnit NOTIFY unitChanged FINAL)
    Q_PROPERTY(float cableLen READ cableLen WRITE setCableLen NOTIFY cableLenChanged FINAL)
    Q_PROPERTY(QString shieldType READ shieldType WRITE setShieldType NOTIFY shieldTypeChanged FINAL)
    Q_PROPERTY(QString imagesPath READ imagesPath CONSTANT FINAL)

public:
    explicit CableTestPara(QObject *parent = nullptr);

    QString selectedCableType() const;
    void setSelectedCableType(const QString &newSelectedCableType);

    int cableCoreCount() const;
    void setCableCoreCount(int newCableCoreCount);

    QString unit() const;
    void setUnit(const QString &newUnit);

    float cableLen() const;
    void setCableLen(float newCableLen);

    QString shieldType() const;
    void setShieldType(const QString &newShieldType);

    QString imagesPath() const;

signals:
    void selectedCableTypeChanged();
    void cableCoreCountChanged();
    void unitChanged();
    void cableLenChanged();
    void shieldTypeChanged();

private:
    QString m_selectedCableType;
    int m_cableCoreCount = 0;
    QString m_unit;
    float m_cableLen = 0.0;
    QString m_shieldType;  // 内屏蔽类型: "A" / "B" / ""
};

#endif // CABLETESTPARA_H
