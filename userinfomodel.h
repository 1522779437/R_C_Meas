#ifndef USERINFOMODEL_H
#define USERINFOMODEL_H

#include <QObject>

class UserInfoModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString outId READ outId WRITE setOutId NOTIFY outIdChanged FINAL)             //出场编号
    Q_PROPERTY(QString spec READ spec WRITE setSpec NOTIFY specChanged FINAL)                 //规格
    Q_PROPERTY(QString outerEnd READ outerEnd WRITE setOuterEnd NOTIFY outerEndChanged FINAL) //外端
    Q_PROPERTY(QString length READ length WRITE setLength NOTIFY lengthChanged FINAL)         //长度

    Q_PROPERTY(QString appearance READ appearance WRITE setAppearance NOTIFY appearanceChanged FINAL) //外观
    Q_PROPERTY(QString leak READ leak WRITE setLeak NOTIFY leakChanged FINAL)                         //是否漏气
    Q_PROPERTY(QString serialNum READ serialNum WRITE setSerialNum NOTIFY serialNumChanged FINAL)     //自编号
    Q_PROPERTY(QString testDate READ testDate WRITE setTestDate NOTIFY testDateChanged FINAL)         //测试日期

    Q_PROPERTY(QString testEnvironment READ testEnvironment WRITE setTestEnvironment NOTIFY testEnvironmentChanged FINAL) //测试环境
    Q_PROPERTY(QString testLocation READ testLocation WRITE setTestLocation NOTIFY testLocationChanged FINAL)             //测试地点
    Q_PROPERTY(QString techDirector READ techDirector WRITE setTechDirector NOTIFY techDirectorChanged FINAL)             //技术负责人
    Q_PROPERTY(QString superUnit READ superUnit WRITE setSuperUnit NOTIFY superUnitChanged FINAL)                         //监理单位

public:
    explicit UserInfoModel(QObject *parent = nullptr);

    QString outId() const;
    void setOutId(const QString &newOutId);

    QString spec() const;
    void setSpec(const QString &newSpec);

    QString outerEnd() const;
    void setOuterEnd(const QString &newOuterEnd);

    QString length() const;
    void setLength(const QString &newLength);

    QString appearance() const;
    void setAppearance(const QString &newAppearance);

    QString leak() const;
    void setLeak(const QString &newLeak);

    QString serialNum() const;
    void setSerialNum(const QString &newSerialNum);

    QString testDate() const;
    void setTestDate(const QString &newTestDate);

    QString testEnvironment() const;
    void setTestEnvironment(const QString &newTestEnvironment);

    QString testLocation() const;
    void setTestLocation(const QString &newTestLocation);

    QString techDirector() const;
    void setTechDirector(const QString &newTechDirector);

    QString superUnit() const;
    void setSuperUnit(const QString &newSuperUnit);

signals:

    void outIdChanged();
    void specChanged();
    void outerEndChanged();
    void lengthChanged();

    void appearanceChanged();

    void leakChanged();

    void serialNumChanged();

    void testDateChanged();

    void testEnvironmentChanged();

    void testLocationChanged();

    void techDirectorChanged();

    void superUnitChanged();

private:
    QString m_outId;
    QString m_spec;
    QString m_outerEnd;
    QString m_length;
    QString m_appearance;
    QString m_leak;
    QString m_serialNum;
    QString m_testDate;
    QString m_testEnvironment;
    QString m_testLocation;
    QString m_techDirector;
    QString m_superUnit;
};

#endif // USERINFOMODEL_H
