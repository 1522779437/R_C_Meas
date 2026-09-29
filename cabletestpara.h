#ifndef CABLETESTPARA_H
#define CABLETESTPARA_H

#include <QObject>
#include <QString>

class CableTestPara : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString selectedCableType READ selectedCableType WRITE setSelectedCableType NOTIFY selectedCableTypeChanged FINAL)
    Q_PROPERTY(int cableCoreCount READ cableCoreCount WRITE setCableCoreCount NOTIFY cableCoreCountChanged FINAL)
    Q_PROPERTY(QString unit READ unit WRITE setUnit NOTIFY unitChanged FINAL)
    Q_PROPERTY(float cableLen READ cableLen WRITE setCableLen NOTIFY cableLenChanged FINAL)

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

signals:
    void selectedCableTypeChanged();
    void cableCoreCountChanged();
    void unitChanged();
    void cableLenChanged();

private:
    QString m_selectedCableType;
    int m_cableCoreCount = 0;
    QString m_unit;
    float m_cableLen = 0.0;
};

#endif // CABLETESTPARA_H
