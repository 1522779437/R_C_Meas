#include "controlpanelmanager.h"
#include "QDebug"
#include "xlsxdocument.h"
#include <QStandardPaths>
#include "userinfomodel.h"
#include <QDir>
#include <QCoreApplication>

ControlPanelManager::ControlPanelManager(QObject *parent)
    : QObject{parent}
{}

bool ControlPanelManager::isWorking() const
{
    return m_isWorking;
}

void ControlPanelManager::setIsWorking(bool newIsWorking)
{
    if (m_isWorking == newIsWorking)
        return;
    m_isWorking = newIsWorking;
    emit isWorkingChanged();
}

// 接口 1 实现
void ControlPanelManager::processStartStop() {
    if (!m_isWorking)
    {
        qDebug() << "C++ 接口: 启动测量。正在初始化硬件并开启数据日志...";
        // 这里添加启动 Modbus 读取等逻辑
        setIsWorking(true);
    }
    else
    {
        qDebug() << "C++ 接口: 暂停测量。正在保持当前状态...";
        setIsWorking(false);
    }
}

// 接口 2 实现
void ControlPanelManager::processEnd()
{
    qDebug() << "C++ 接口: 强制结束测量。正在关闭继电器并生成临时缓存...";
    setIsWorking(false);
    // 这里添加硬件复位逻辑
}

// 接口 3 实现：导出 Excel 报表
void ControlPanelManager::processExport(QObject* infoObj) {
    qDebug() << "C++ 接口: 启动 QXlsx 导出，正在绘制标准记录表...";

    // 0. 模型转换与安全检查
    UserInfoModel* info = qobject_cast<UserInfoModel*>(infoObj);
    if (!info) {
        qDebug() << "导出失败：传入的不是有效的 UserInfoModel 对象！";
        emit exportFinished(false, "数据模型错误");
        return;
    }

    // 1. 创建 Excel 文档对象
    QXlsx::Document xlsx;

    // --- 样式定义 ---
    // 标题样式 (Row 1)
    QXlsx::Format titleFormat;
    titleFormat.setFontSize(16);
    titleFormat.setFontBold(true);
    titleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    titleFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 副标题样式 (Row 2)
    QXlsx::Format subTitleFormat;
    subTitleFormat.setFontSize(14);
    subTitleFormat.setFontBold(true);
    subTitleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    subTitleFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 基础文字样式（用于 Row 3, 4）
    QXlsx::Format textFormat;
    textFormat.setFontSize(11);
    textFormat.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    textFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 表格表头样式（居中 + 细边框）
    QXlsx::Format headerFormat;
    headerFormat.setFontSize(11);
    headerFormat.setFontBold(true);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);

    // 数据区样式
    QXlsx::Format dataFormat;
    dataFormat.setBorderStyle(QXlsx::Format::BorderThin);
    dataFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    dataFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 底部说明样式（换行显示）
    QXlsx::Format footerFormat;
    footerFormat.setFontSize(10);
    footerFormat.setTextWrap(true);
    footerFormat.setVerticalAlignment(QXlsx::Format::AlignTop);
    footerFormat.setHorizontalAlignment(QXlsx::Format::AlignLeft);

    // 底部签字左对齐
    QXlsx::Format signLeftFormat;
    signLeftFormat.setFontSize(11);
    signLeftFormat.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    signLeftFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 底部签字右对齐
    QXlsx::Format signRightFormat;
    signRightFormat.setFontSize(11);
    signRightFormat.setHorizontalAlignment(QXlsx::Format::AlignRight);
    signRightFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // --- 开始绘制表头 ---
    xlsx.write("A1", "中铁电气化局集团有限公司沈阳电气化工程分公司沈大线联锁设备改造工程项目部", titleFormat);
    xlsx.mergeCells("A1:I1");
    xlsx.setRowHeight(1, 30);

    xlsx.write("A2", "                    电缆单盘记录表                 沈大线信01", subTitleFormat);
    xlsx.mergeCells("A2:I2");
    xlsx.setRowHeight(2, 25);

    QString row3 = QString("出厂编号: %1       规格: %2       外端: %3       长度: %4       外观: %5       是否漏气: %6")
                       .arg(info->outId(), info->spec(), info->outerEnd(), info->length(), info->appearance(), info->leak());
    xlsx.write("A3", row3, textFormat);
    xlsx.mergeCells("A3:I3");

    QString row4 = QString("自编号: %1         测试日期: %2         测试环境: %3         测试地点: %4")
                       .arg(info->serialNum(), info->testDate(), info->testEnvironment(), info->testLocation());
    xlsx.write("A4", row4, textFormat);
    xlsx.mergeCells("A4:I4");

    // --- 绘制表格结构 ---
    xlsx.write("A5", "四芯组", headerFormat); xlsx.mergeCells("A5:A46");
    xlsx.write("B5", "组别", headerFormat);   xlsx.mergeCells("B5:B6");
    xlsx.write("C5", "色别", headerFormat);   xlsx.mergeCells("C5:C6");
    xlsx.write("D5", "颜色", headerFormat);   xlsx.mergeCells("D5:D6");
    xlsx.write("E5", "直流电阻   Ω", headerFormat); xlsx.mergeCells("E5:E6");

    xlsx.write("F5", "绝缘电阻   MΩ", headerFormat); xlsx.mergeCells("F5:G5");
    xlsx.write("F6", "对地", headerFormat);
    xlsx.write("G6", "线间", headerFormat);

    xlsx.write("H5", "电阻不平衡%", headerFormat); xlsx.mergeCells("H5:H6");
    xlsx.write("I5", "工作电容(nf)", headerFormat); xlsx.mergeCells("I5:I6");

    // 数据边框绘制 (Row 7-58)
    for(int r = 7; r <= 58; ++r) {
        for(int c = 2; c <= 9; ++c) {
            xlsx.write(r, c, "", dataFormat);
        }
    }

    // --- 绘制底部区域 ---
    xlsx.write("A47", "对绞  单芯", headerFormat);
    xlsx.mergeCells("A47:A58");

    QString description =
        "       20℃时电缆长度为1000米的标准值:\n"
        "       导线的直流电阻值为23.5±1Ω,普通电缆绝缘电阻值不小于3000MΩ，电阻不平衡系数不大于2%，\n"
        "       四芯组线间工作电容值50nf/km，对绞组线间工作电容值为70nf/km；\n"
        "       单根芯线対连到地的其它绝缘芯线间电容不大于100nf/km；\n"
        "       数字电缆绝缘电阻值不小于10000MΩ,四芯组线间工作电容值28+2nf/km,对绞组线间工作电容值为35+4nf/km;\n"
        "       单根芯线对连到地的其他绝缘芯线间电容不大于70nf/km,电阻不平衡系数不大于1%;\n"
        "       本盘电缆经换算成长度为1000米时各项数值。";

    xlsx.write("A59", description, footerFormat);
    xlsx.mergeCells("A59:I65");
    xlsx.setRowHeight(59, 110);

    // 绘制最后一行签字位：两端分布
    xlsx.write(66, 1, QString("  技术负责人：%1").arg(info->techDirector()), signLeftFormat);
    xlsx.mergeCells("A66:E66"); // 左侧占 5 列
    xlsx.write(66, 6, QString("监理单位：%1  ").arg(info->superUnit()), signLeftFormat);
    xlsx.mergeCells("F66:I66"); // 右侧占 4 列
    xlsx.setRowHeight(66, 30);

    // --- 设置列宽 ---
    xlsx.setColumnWidth(1, 10);
    xlsx.setColumnWidth(2, 6);
    xlsx.setColumnWidth(3, 6);
    xlsx.setColumnWidth(4, 6);
    xlsx.setColumnWidth(5, 16);
    xlsx.setColumnWidth(6, 12);
    xlsx.setColumnWidth(7, 12);
    xlsx.setColumnWidth(8, 14);
    xlsx.setColumnWidth(9, 14);

    // --- 自动获取路径并执行保存 ---
    // 自动获取当前 .cpp 所在的源码目录
    QFileInfo cppFileInfo(QString::fromLocal8Bit(__FILE__));
    QString projectPath = cppFileInfo.absolutePath();
    QDir dir(projectPath);

    if (!dir.exists("export")) {
        dir.mkdir("export");
    }
    dir.cd("export");

    QString timeStr = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString fileName = QString("电缆测试记录表_%1.xlsx").arg(timeStr);
    QString savePath = dir.absoluteFilePath(fileName);

    if (xlsx.saveAs(savePath)) {
        qDebug() << "Excel 成功保存至:" << savePath;
        emit exportFinished(true, savePath);
    } else {
        qDebug() << "保存失败！";
        emit exportFinished(false, "文件保存失败");
    }
}
