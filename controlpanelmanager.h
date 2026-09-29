#ifndef CONTROLPANELMANAGER_H
#define CONTROLPANELMANAGER_H

#include <QObject>
#include "userinfomodel.h"

class ControlPanelManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isWorking READ isWorking WRITE setIsWorking NOTIFY isWorkingChanged FINAL)

public:
    explicit ControlPanelManager(QObject *parent = nullptr);

    bool isWorking() const;
    void setIsWorking(bool newIsWorking);

    // --- 接口 1: 处理开始/停止逻辑 ---
    Q_INVOKABLE void processStartStop();

    // --- 接口 2: 处理结束逻辑 (强制复位/保存) ---
    Q_INVOKABLE void processEnd();

    // --- 接口 3: 处理导出逻辑 ---
    Q_INVOKABLE void processExport(QObject* infoObj);

signals:
    void isWorkingChanged();
    void exportFinished(bool success, QString filePath);
private:
    bool m_isWorking = false;
};

#endif // CONTROLPANELMANAGER_H
