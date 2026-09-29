#include "controlpanelmanager.h"
#include "QDebug"
#include "xlsxdocument.h"
#include <QStandardPaths>
#include <QRegularExpression>
#include "userinfomodel.h"
#include <QDir>
#include <QCoreApplication>

ControlPanelManager::ControlPanelManager(QObject *parent)
    : QObject{parent}
{
    m_timer = new QTimer(this);
    m_timer->stop();
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(MACHINE_STATE_UPDATE_TIME);
    m_timer->setSingleShot(false);
    QMetaObject::Connection conn = connect(m_timer, &QTimer::timeout, this, &ControlPanelManager::state_change);
    qDebug() << "ControlPanelManager 构造完成, timer 连接状态:" << (bool)conn;
    measure_state = INIT_DEVS;   // 从头开始状态机

}

ControlPanelManager::~ControlPanelManager()
{
    m_timer->stop();

    // 释放 MODBUS (COM6)
    if (modbusDevice)
    {
        modbusDevice->disconnectDevice();
        modbusDevice->deleteLater();
        modbusDevice = nullptr;
    }

    // 释放标准串口 (COM7)
    if (m_comPort)
    {
        if (m_comPort->isOpen())
            m_comPort->close();
        m_comPort->deleteLater();
        m_comPort = nullptr;
    }

    qDebug() << "ControlPanelManager: 串口资源已释放";
}

bool ControlPanelManager::isWorking() const
{
    return m_workState == WORKING;  // 兼容旧的 bool 属性，给 QML 用
}

int ControlPanelManager::workState() const
{
    return m_workState;
}

void ControlPanelManager::setTestConfig(TestProjectConfig* config)
{
    m_testConfig = config;
}

void ControlPanelManager::setCablePara(CableTestPara* para)
{
    m_cablePara = para;
}

void ControlPanelManager::setWorkState(WorkState newState)
{
    if (m_workState == newState)
        return;
    m_workState = newState;
    emit workStateChanged();
    emit isWorkingChanged();  // 兼容旧信号
}

// 接口 1 实现
void ControlPanelManager::processStartStop() {
    switch (m_workState)
    {
    case IDLE:
        qDebug() << "C++ 接口: 启动测量。正在初始化硬件并开启数据日志...";
        if (m_cablePara)
            qDebug() << "  电缆类型:" << m_cablePara->selectedCableType()
                     << "| 电缆芯数:" << m_cablePara->cableCoreCount()
                     << "| 电缆长度:" << m_cablePara->cableLen() << m_cablePara->unit();
        // 校验：最多支持3块板(48芯)
        if (m_cablePara && m_cablePara->cableCoreCount() > 48) {
            qDebug() << "ERROR: 电缆芯数" << m_cablePara->cableCoreCount() << "超过最大支持48芯!";
            return;
        }
        group = m_cablePara->cableCoreCount() / 16;
        number = m_cablePara->cableCoreCount() % 16;
        m_dcMeasured = false;              // 重置已测量标记
        m_insMeasured = false;
        m_capMeasured = false;
        m_foundValidDc = false;            // 重置快速跳线标志
        m_measData.clear();                // 清空上次测量缓存
        emit clearResults();               // 清空测试结果表
        measure_state = INIT_DEVS;           // 每次从IDLE启动都重新初始化设备
        setWorkState(WORKING);
        m_timer->start();
        break;

    case WORKING:
        qDebug() << "C++ 接口: 暂停测量。正在保持当前状态...";
        setWorkState(PAUSED);
        m_timer->stop();
        break;

    case PAUSED:
        qDebug() << "C++ 接口: 恢复测量。正在继续...";
        setWorkState(WORKING);
        m_timer->start();
        break;
    }
}

// 接口 2 实现
void ControlPanelManager::processEnd()
{
    qDebug() << "C++ 接口: 强制结束测量。正在关闭继电器并生成临时缓存...";
    setWorkState(IDLE);
    m_timer->stop();
    resetState();
    // 这里添加硬件复位逻辑
}

// 接口 4: 重测选中行 —— 根据勾选的测试项目决定重测内容
void ControlPanelManager::startRetest(QVariantList positions)
{
    m_retestList.clear();
    for (const QVariant &v : positions)
        m_retestList.append(v.toInt());

    if (m_retestList.isEmpty())
    {
        qDebug() << "startRetest: 重测列表为空，取消";
        return;
    }

    qDebug() << "startRetest: 重测" << m_retestList.size() << "个位置:" << m_retestList;

    // 根据当前勾选的测试项目决定重测内容
    m_retestDoDC = m_testConfig && m_testConfig->checkDC();
    m_retestDoIns = m_testConfig && m_testConfig->checkIns();
    m_retestDoCap = m_testConfig && m_testConfig->checkCap();

    m_isRetestMode = true;
    m_retestJustDone = false;
    m_retestIndex = 0;
    m_foundValidDc = false;
    m_dcMeasured = false;
    m_insMeasured = false;

    // 重新计算电缆参数（resetState 后 group/number 可能已被清零）
    group = m_cablePara->cableCoreCount() / 16;
    number = m_cablePara->cableCoreCount() % 16;

    // 复位测量相关变量
    switch_over = false;
    switch_once_over = false;
    is_stable = false;
    skip_r_flag = false;
    total_try_count = 0;
    consecutive_count = 0;
    last_range = -1;
    r_module_range = 0;
    m_insOvldRetry = 0;
    m_serialConfigSucc = false;

    // 复位所有继电器编号
    wire_left_selection_1_number = 0;
    wire_left_selection_2_number = 0;
    wire_left_selection_3_number = 0;
    wire_right_selection_1_number = 0;
    wire_right_selection_2_number = 0;
    wire_right_selection_3_number = 0;

    // 直接定位到第一个重测位置
    int firstPos = m_retestList[0];
    int newBoard = (firstPos - 1) / 16;
    int newNumber = (firstPos - 1) % 16;

    wire_left_selection_board = newBoard;
    wire_right_selection_board = newBoard;

    if (newBoard == 0)
    {
        wire_left_selection_1_number = newNumber;
        wire_right_selection_1_number = newNumber;
    }
    else if (newBoard == 1)
    {
        wire_left_selection_2_number = newNumber;
        wire_right_selection_2_number = newNumber;
    }
    else if (newBoard == 2)
    {
        wire_left_selection_3_number = newNumber;
        wire_right_selection_3_number = newNumber;
    }

    emit retestModeChanged();

    // 决定第一个重测项目并启动状态机
    if (m_retestDoDC)
    {
        m_measuringDC = true;
        m_measuringIns = false;
        m_measuringCap = false;
        m_syncMode = true;
        setTimerInterval(DC_STATE_UPDATE_TIME);
        measure_state = TO_LOW_R_MODULE;
        qDebug() << "startRetest: 首个重测项目 → 直流电阻，位置" << firstPos;
    }
    else if (m_retestDoIns)
    {
        m_measuringDC = false;
        m_measuringIns = true;
        m_measuringCap = false;
        m_syncMode = false;
        setTimerInterval(INS_STATE_UPDATE_TIME);
        measure_state = MODULE_SELECTION_TO_HIGH_R;
        qDebug() << "startRetest: 首个重测项目 → 绝缘电阻，位置" << firstPos;
    }
    else if (m_retestDoCap)
    {
        m_measuringDC = false;
        m_measuringIns = false;
        m_measuringCap = true;
        m_syncMode = false;
        setTimerInterval(CAP_STATE_UPDATE_TIME);
        m_capPairIndex = (firstPos - 1) / 2;  // 从首个重测位置的 pair 开始
        measure_state = CONFIG_C_MODULE;
        qDebug() << "startRetest: 首个重测项目 → 工作电容，位置" << firstPos
                 << "Pair" << m_capPairIndex;
    }

    setWorkState(WORKING);
    m_timer->start();
}

void ControlPanelManager::resetState()
{
    measure_state = MAKE_RECORD_FILE;
    m_dcMeasured = false;
    m_insMeasured = false;
    m_capMeasured = false;
    m_measuringDC = false;
    m_measuringIns = false;
    m_measuringCap = false;
    m_foundValidDc = false;

    switch_over = false;
    switch_once_over = false;

    // 左侧继电器
    wire_left_selection_1_state = 0x0000;
    wire_left_selection_2_state = 0x0000;
    wire_left_selection_3_state = 0x0000;
    wire_left_selection_board = 0;
    wire_left_selection_1_number = 0x0000;
    wire_left_selection_2_number = 0x0000;
    wire_left_selection_3_number = 0x0000;
    wire_left_selection_1_flag = true;
    wire_left_selection_2_flag = true;
    wire_left_selection_3_flag = true;

    // 右侧继电器
    wire_right_selection_1_state = 0x0000;
    wire_right_selection_2_state = 0x0000;
    wire_right_selection_3_state = 0x0000;
    wire_right_selection_board = 0;
    wire_right_selection_1_number = 0x0000;
    wire_right_selection_2_number = 0x0000;
    wire_right_selection_3_number = 0x0000;
    wire_right_selection_1_flag = true;
    wire_right_selection_2_flag = true;
    wire_right_selection_3_flag = true;

    // 电阻测量相关
    total_try_count = 0;
    last_range = -1;
    consecutive_count = 0;
    ref_value = 0.0f;
    is_stable = false;
    skip_r_flag = false;
    m_insOvldRetry = 0;
    m_chargeTicks = 0;
    m_serialConfigSucc = false;
    m_capMeasureSucc = false;
    m_capPairIndex = 0;
    m_capAttemptCount = 0;
    m_capOlCount = 0;
    m_capValidCount = 0;
    m_capStableCount = 0;
    m_capBaselineNf = -1.0;
    m_capSum_F = 0.0;
    m_capValue = -1.0;

    // 测量数据
    modebus_value = 0;
    r_module_range = 0;
    // group 和 number 在 processStartStop 里重新设置，这里也清零
    group = 0;
    number = 0;

    // 重测相关
    m_isRetestMode = false;
    m_retestJustDone = false;
    m_retestDoDC = false;
    m_retestDoIns = false;
    m_retestDoCap = false;
    m_retestList.clear();
    m_retestIndex = 0;
    m_syncMode = false;

    qDebug() << "resetState: 所有测量中间变量已清零";
}

void ControlPanelManager::setTimerInterval(int ms)
{
    if (m_timer)
    {
        m_timer->setInterval(ms);
        qDebug() << "setTimerInterval: 定时器间隔 →" << ms << "ms";
    }
    if (modbusDevice)
    {
        int timeout = ms - 10;
        if (timeout < 10) timeout = 10;  // 最小 10ms 保底
        modbusDevice->setTimeout(timeout);
        qDebug() << "setTimerInterval: Modbus超时 →" << timeout << "ms";
    }
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
    titleFormat.setFontName("宋体");
    titleFormat.setFontSize(12);
    titleFormat.setFontBold(true);
    titleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    titleFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 副标题样式 (Row 2)
    QXlsx::Format subTitleFormat;
    subTitleFormat.setFontSize(14);
    subTitleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    subTitleFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 基础文字样式（用于 Row 3, 4）
    QXlsx::Format textFormat;
    textFormat.setFontSize(12);
    textFormat.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    textFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 表格表头样式（居中 + 细边框）
    QXlsx::Format headerFormat;
    headerFormat.setFontSize(10);
    headerFormat.setFontBold(false);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);

    // A列分组标签样式（"四芯组"、"对绞 单芯"）
    QXlsx::Format groupLabelFormat;
    groupLabelFormat.setFontSize(12);
    groupLabelFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    groupLabelFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    groupLabelFormat.setBorderStyle(QXlsx::Format::BorderThin);

    // 数据区样式
    QXlsx::Format dataFormat;
    dataFormat.setBorderStyle(QXlsx::Format::BorderThin);
    dataFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    dataFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // 底部说明样式（换行显示）
    QXlsx::Format footerFormat;
    footerFormat.setFontSize(12);
    footerFormat.setTextWrap(true);
    footerFormat.setVerticalAlignment(QXlsx::Format::AlignTop);
    footerFormat.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    // 👇 新增这一行：设置边框为细实线（黑色）
    footerFormat.setBorderStyle(QXlsx::Format::BorderThin);

    // 底部签字左对齐
    QXlsx::Format signLeftFormat;
    signLeftFormat.setFontSize(12);
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
    xlsx.write("A5", "四芯组", groupLabelFormat);
    xlsx.mergeCells("A5:A54", groupLabelFormat);

    xlsx.write("B5", "组别", headerFormat);   
    xlsx.mergeCells("B5:B6", headerFormat);
    
    xlsx.write("C5", "色别", headerFormat);   
    xlsx.mergeCells("C5:C6", headerFormat);
    
    xlsx.write("D5", "颜色", headerFormat);   
    xlsx.mergeCells("D5:D6", headerFormat);
    
    xlsx.write("E5", "直流电阻  Ω", headerFormat); 
    xlsx.mergeCells("E5:E6", headerFormat);

    xlsx.write("F5", "绝缘电阻  MΩ", headerFormat); 
    xlsx.mergeCells("F5:G5", headerFormat); // 确保 G5 也有上边框和右边框
    xlsx.write("F6", "对地", headerFormat);
    xlsx.write("G6", "线间", headerFormat);

    xlsx.write("H5", "电阻不平衡%", headerFormat); 
    xlsx.mergeCells("H5:H6", headerFormat);
    
    xlsx.write("I5", "工作电容(nf)", headerFormat); 
    xlsx.mergeCells("I5:I6", headerFormat);

    // 数据边框绘制 (Row 7-54 四芯组, Row 55-70 对绞单芯)
    for(int r = 7; r <= 70; ++r) {
        for(int c = 2; c <= 9; ++c) {
            xlsx.write(r, c, "", dataFormat);
        }
    }

    int signalGroups = 0;
    int signalPairCount = 0;

    // ===== 填充四芯组 + 对绞/单芯标签 (信号电缆，按芯数独立处理) =====
    if (m_cablePara && m_cablePara->selectedCableType() == "信号电缆") {
        static const QStringList ROMAN = {"I","II","III","IV","V","VI","VII","VIII","IX","X","XI","XII"};
        static const QStringList GROUP4_COLORS = {"红","白","蓝","绿"};

        int coreCount = m_cablePara->cableCoreCount();
        int groups = 0;                      // 四芯组数
        QStringList groupColors;             // 四芯组每组的色别(C列), 空则默认"红"
        QStringList pairLabels;              // 对绞/单芯每行C列标签
        QStringList pairColors;              // 对绞/单芯每行D列颜色

        switch (coreCount) {
        case 4:  groups = 1;  break;
        case 6:  groups = 0;  pairColors = {"红","白","绿","白","蓝","绿"};              break;
        case 8:  groups = 0;  pairColors = {"红","白","绿","白","蓝","绿","蓝","白"};      break;
        case 9:  groups = 0;
                 pairLabels = {"D-1","D-2","D-3","D-4","D-5","D-6","D-7","D-8","1"};
                 pairColors = {"红","白","绿","白","蓝","绿","蓝","白","红"};              break;
        case 12: groups = 3;  groupColors = {"红","绿","白"};                             break;
        case 14: groups = 3;  groupColors = {"红","绿","白"};
                 pairLabels = {"1","2"};  pairColors = {"红","绿"};                        break;
        case 16: groups = 4;  groupColors = {"红","绿","白","蓝"};                         break;
        case 19: groups = 4;  groupColors = {"红","绿","白","蓝"};
                 pairLabels = {"1","2","3"};  pairColors = {"红","绿","白"};               break;
        case 21: groups = 4;  groupColors = {"红","绿","白","蓝"};
                 pairLabels = {"1","2","3","4","5"};  pairColors = {"红","绿","白","蓝","白"}; break;
        case 24: groups = 5;  groupColors = {"红","绿","白","蓝","白"};
                 pairLabels = {"1","2","D-1","D-2"};  pairColors = {"红","绿","红","白"};   break;
        case 28: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};           break;
        case 30: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2"};  pairColors = {"红","绿"};                        break;
        case 33: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","4","5"};  pairColors = {"红","绿","白","蓝","白"}; break;
        case 37: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","D-1","D-2","D-3","D-4","D-5","D-6"};
                 pairColors = {"红","绿","白","红","白","绿","白","蓝","绿"};               break;
        case 42: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","4","5","6","D-1","D-2","D-3","D-4","D-5","D-6","D-7","D-8"};
                 pairColors = {"红","绿","白","蓝","白","蓝","红","白","绿","白","蓝","绿","蓝","白"}; break;
        case 44: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","4","5","6","7","8","D-1","D-2","D-3","D-4","D-5","D-6","D-7","D-8"};
                 pairColors = {"红","绿","白","蓝","白","蓝","白","蓝","红","白","绿","白","蓝","绿","蓝","白"}; break;
        case 48: groups = 12; groupColors = {"红","绿","白","蓝","白","蓝","白","蓝","白","蓝","白","蓝"}; break;
        default: groups = coreCount / 4;  break;
        }

        signalGroups = groups;
        signalPairCount = pairColors.size();

        QXlsx::Format labelFormat;
        labelFormat.setFontSize(10);
        labelFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        labelFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
        labelFormat.setBorderStyle(QXlsx::Format::BorderThin);

        // 四芯组
        for (int g = 0; g < groups && g < 12; ++g) {
            int startRow = 7 + g * 4;
            xlsx.write(startRow, 2, ROMAN.value(g, ""), labelFormat); // B列: 组别
            QString cTag = (g < groupColors.size()) ? groupColors[g] : "红";
            xlsx.write(startRow, 3, cTag, labelFormat);               // C列: 色别
            for (int w = 0; w < 4; ++w) {
                xlsx.write(startRow + w, 4, GROUP4_COLORS[w], labelFormat); // D列: 颜色
            }
        }

        // 对绞/单芯
        for (int i = 0; i < pairColors.size() && i < 16; ++i) {
            int r = 55 + i;
            QString cLabel = (i < pairLabels.size()) ? pairLabels[i] : QString("D-%1").arg(i + 1);
            xlsx.write(r, 3, cLabel, labelFormat);       // C列: 色别
            xlsx.write(r, 4, pairColors[i], labelFormat); // D列: 颜色
        }
    }

    // B列组别、C列色别：四芯组每4行合并 (12组，行7-54)
    // H列电阻不平衡率、I列工作电容：每2行合并 (行7-54)
    for (int g = 0; g < 12; ++g) {
        int startRow = 7 + g * 4;
        int endRow = startRow + 3;
        xlsx.mergeCells(QString("B%1:B%2").arg(startRow).arg(endRow));
        xlsx.mergeCells(QString("C%1:C%2").arg(startRow).arg(endRow));
    }
    for (int r = 7; r <= 53; r += 2) {
        xlsx.mergeCells(QString("H%1:H%2").arg(r).arg(r + 1));
        xlsx.mergeCells(QString("I%1:I%2").arg(r).arg(r + 1));
    }

    // ===== 填充四芯组 + 对绞/单芯标签 (内屏蔽数字信号电缆) =====
    if (m_cablePara && m_cablePara->selectedCableType() == "内屏蔽数字信号电缆") {
        QString shield = m_cablePara->shieldType();  // "A" / "B"

        int coreCount = m_cablePara->cableCoreCount();
        int groups = 0;
        QStringList groupColors;
        QStringList pairLabels;
        QStringList pairColors;

        // 按芯数+屏蔽类型独立配置
        if (coreCount == 8 && shield == "B") {
            groups = 2;  groupColors = {"红","绿"};
        } else if (coreCount == 12) {
            groups = 3;  groupColors = {"红","绿","白"};
        } else if (coreCount == 14) {
            groups = 3;  groupColors = {"红","绿","白"};
            pairLabels = {"1","2"};  pairColors = {"红","绿"};
        } else if (coreCount == 16) {
            groups = 4;  groupColors = {"红","绿","白","蓝"};
        } else if (coreCount == 19) {
            groups = 4;  groupColors = {"红","绿","白","蓝"};
            pairLabels = {"1","2","3"};  pairColors = {"红","绿","白"};
        } else if (coreCount == 21) {
            groups = 5;  groupColors = {"红","绿","白","蓝","白"};
            pairLabels = {"1"};  pairColors = {"红"};
        } else if (coreCount == 24) {
            groups = 6;
            pairLabels.clear();  pairColors.clear();
            if (shield == "A")
                groupColors = {"红","绿","白","蓝","白","红/蓝"};
            else
                groupColors = {"红","绿","白","蓝","白","蓝"};
        } else if (coreCount == 28) {
            groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","红/蓝"};
        } else if (coreCount == 30) {
            groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","红/蓝"};
            pairLabels = {"1","2"};  pairColors = {"红","绿"};
        } else if (coreCount == 33) {
            groups = 8;  groupColors = {"红","绿","白","蓝","白","蓝","白","红/蓝"};
            pairLabels = {"1"};  pairColors = {"红"};
        } else if (coreCount == 37) {
            groups = 9;  groupColors = {"红","绿","白","蓝","白","蓝","白","蓝","红/蓝"};
            pairLabels = {"1"};  pairColors = {"红"};
        } else if (coreCount == 42) {
            groups = 10; groupColors = {"红","绿","白","蓝","白","蓝","白","蓝","绿","白"};
            pairLabels = {"1","2"};  pairColors = {"红","绿"};
        } else if (coreCount == 44) {
            groups = 11; groupColors = {"红","绿","白","蓝","白","蓝","白","红","绿","白","蓝"};
        } else if (coreCount == 48) {
            groups = 12; groupColors = {"红","绿","白","蓝","白","蓝","白","蓝","红","绿","白","蓝"};
        }

        QXlsx::Format labelFormat;
        labelFormat.setFontSize(10);
        labelFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        labelFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
        labelFormat.setBorderStyle(QXlsx::Format::BorderThin);

        static const QStringList ROMAN = {"I","II","III","IV","V","VI","VII","VIII","IX","X","XI","XII"};
        static const QStringList GROUP4_COLORS = {"红","白","蓝","绿"};

        // 四芯组
        for (int g = 0; g < groups && g < 12; ++g) {
            int startRow = 7 + g * 4;
            xlsx.write(startRow, 2, ROMAN.value(g, ""), labelFormat);
            QString cTag = (g < groupColors.size()) ? groupColors[g] : "红";
            xlsx.write(startRow, 3, cTag, labelFormat);
            for (int w = 0; w < 4; ++w) {
                xlsx.write(startRow + w, 4, GROUP4_COLORS[w], labelFormat);
            }
        }

        // 对绞/单芯
        for (int i = 0; i < pairColors.size() && i < 16; ++i) {
            int r = 55 + i;
            QString cLabel = (i < pairLabels.size()) ? pairLabels[i] : QString("D-%1").arg(i + 1);
            xlsx.write(r, 3, cLabel, labelFormat);
            xlsx.write(r, 4, pairColors[i], labelFormat);
        }

        signalGroups = groups;
        signalPairCount = pairColors.size();
    }

    // ===== 填充四芯组 + 对绞/单芯标签 (数字信号电缆，按芯数独立处理) =====
    if (m_cablePara && m_cablePara->selectedCableType() == "数字信号电缆") {
        static const QStringList ROMAN2 = {"I","II","III","IV","V","VI","VII","VIII","IX","X","XI","XII"};
        static const QStringList GROUP4_COLORS2 = {"红","白","蓝","绿"};

        int coreCount = m_cablePara->cableCoreCount();
        int groups = 0;
        QStringList groupColors;
        QStringList pairLabels;
        QStringList pairColors;

        switch (coreCount) {
        case 4:  groups = 1;  groupColors = {"红"};  break;
        case 6:  groups = 0;  pairColors = {"红","白","绿","白","蓝","绿"};  break;
        case 8:  groups = 2;  groupColors = {"红","绿"};  break;
        case 9:  groups = 2;  groupColors = {"红","绿"};
                 pairLabels = {"1"};  pairColors = {"红"};  break;
        case 12: groups = 3;  groupColors = {"红","绿","白"};  break;
        case 14: groups = 3;  groupColors = {"红","绿","白"};
                 pairLabels = {"1","2"};  pairColors = {"红","绿"};  break;
        case 16: groups = 4;  groupColors = {"红","绿","白","蓝"};  break;
        case 19: groups = 4;  groupColors = {"红","绿","白","蓝"};
                 pairLabels = {"1","2","3"};  pairColors = {"红","绿","白"};  break;
        case 21: groups = 5;  groupColors = {"红","绿","白","蓝","白"};
                 pairLabels = {"1"};  pairColors = {"红"};  break;
        case 24: groups = 6;  groupColors = {"红","绿","白","蓝","白","蓝"};  break;
        case 28: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};  break;
        case 30: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2"};  pairColors = {"红","绿"};  break;
        case 33: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","4","5"};  pairColors = {"红","绿","白","蓝","白"};  break;
        case 37: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","D-1","D-2","D-3","D-4","D-5","D-6"};
                 pairColors = {"红","绿","白","红","白","绿","白","蓝","绿"};  break;
        case 42: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","4","5","6","D-1","D-2","D-3","D-4","D-5","D-6","D-7","D-8"};
                 pairColors = {"红","绿","白","蓝","白","蓝","红","白","绿","白","蓝","绿","蓝","白"};  break;
        case 44: groups = 7;  groupColors = {"红","绿","白","蓝","白","蓝","白"};
                 pairLabels = {"1","2","3","4","5","6","7","8","D-1","D-2","D-3","D-4","D-5","D-6","D-7","D-8"};
                 pairColors = {"红","绿","白","蓝","白","蓝","白","蓝","红","白","绿","白","蓝","绿","蓝","白"};  break;
        case 48: groups = 12; groupColors = {"红","绿","白","蓝","白","蓝","白","蓝","白","蓝","白","蓝"};  break;
        default: groups = coreCount / 4;  break;
        }

        QXlsx::Format labelFormat;
        labelFormat.setFontSize(10);
        labelFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        labelFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
        labelFormat.setBorderStyle(QXlsx::Format::BorderThin);

        // 四芯组
        for (int g = 0; g < groups && g < 12; ++g) {
            int startRow = 7 + g * 4;
            xlsx.write(startRow, 2, ROMAN2.value(g, ""), labelFormat);
            QString cTag = (g < groupColors.size()) ? groupColors[g] : "红";
            xlsx.write(startRow, 3, cTag, labelFormat);
            for (int w = 0; w < 4; ++w) {
                xlsx.write(startRow + w, 4, GROUP4_COLORS2[w], labelFormat);
            }
        }

        // 对绞/单芯
        for (int i = 0; i < pairColors.size() && i < 16; ++i) {
            int r = 55 + i;
            QString cLabel = (i < pairLabels.size()) ? pairLabels[i] : QString("D-%1").arg(i + 1);
            xlsx.write(r, 3, cLabel, labelFormat);
            xlsx.write(r, 4, pairColors[i], labelFormat);
        }

        signalGroups = groups;
        signalPairCount = pairColors.size();
    }

    // ===== 其它电缆：D7~D54 依次填1~48，BC列不填 =====
    if (m_cablePara && m_cablePara->selectedCableType() == "其它电缆") {
        QXlsx::Format labelFormat;
        labelFormat.setFontSize(10);
        labelFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        labelFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
        labelFormat.setBorderStyle(QXlsx::Format::BorderThin);

        int coreCount = m_cablePara->cableCoreCount();
        int groups = (coreCount + 3) / 4;  // 向上取整，确保所有线都在四芯组区域

        // D列颜色：D7~D54 依次填入 1~48
        for (int i = 0; i < 48; ++i) {
            xlsx.write(7 + i, 4, QString::number(i + 1), labelFormat);
        }

        signalGroups = groups;
        signalPairCount = 0;
    }

    // ===== 填充测量数据 =====
    qDebug() << "导出: m_measData 条目数 =" << m_measData.size();
    QXlsx::Format measDataFormat;
    measDataFormat.setFontSize(10);
    measDataFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    measDataFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    measDataFormat.setBorderStyle(QXlsx::Format::BorderThin);

    for (auto it = m_measData.constBegin(); it != m_measData.constEnd(); ++it) {
        int pos = it.key();
        const MeasRow& row = it.value();
        // 位置映射到Excel行：1~groups*4 → 四芯组(row 7+)，之后 → 对绞/单芯(row 55+)
        int excelRow;
        int groupWireCount = signalGroups * 4;
        if (pos <= groupWireCount)
            excelRow = 7 + (pos - 1);
        else
            excelRow = 55 + (pos - groupWireCount - 1);
        if (excelRow < 7 || excelRow > 70) continue;

        // E列: 直流电阻 Ω (保留两位小数)
        if (row.hasDC) {
            if (row.dc >= 0)
                xlsx.write(excelRow, 5, QString::number(row.dc, 'f', 2), measDataFormat);
            else
                xlsx.write(excelRow, 5, "OVLD", measDataFormat);
        }

        // F列: 绝缘对地 MΩ (保留两位小数)
        if (row.hasIns) {
            if (row.insGND >= 0)
                xlsx.write(excelRow, 6, QString::number(row.insGND, 'f', 2), measDataFormat);
            else
                xlsx.write(excelRow, 6, "OVLD", measDataFormat);
        }

        // G列: 绝缘线间 MΩ (保留两位小数)
        if (row.hasIns) {
            if (row.insL1 >= 0)
                xlsx.write(excelRow, 7, QString::number(row.insL1, 'f', 2), measDataFormat);
            else
                xlsx.write(excelRow, 7, "OVLD", measDataFormat);
        }

        // I列: 工作电容 nF — 仅四芯组区域且奇数行写入 (对绞/单芯不记录电容)
        if (row.hasCap && (pos % 2 != 0) && pos <= signalGroups * 4) {
            if (row.cap >= 0)
                xlsx.write(excelRow, 9, QString::number(row.cap, 'f', 3), measDataFormat);
            else
                xlsx.write(excelRow, 9, "OVLD", measDataFormat);
        }
    }

    // H列: 电阻不平衡率 (所有电缆类型基于四芯组配对)
    for (int g = 0; g < signalGroups && g < 12; ++g) {
            int basePos = g * 4 + 1;  // 每组起始位置: 1, 5, 9, ...
            for (int pair = 0; pair < 2; ++pair) {
                int pOdd = basePos + pair * 2;
                int pEven = pOdd + 1;
                double r1 = m_measData.value(pOdd).dc;
                double r2 = m_measData.value(pEven).dc;
                if (r1 > 0 && r2 > 0) {
                    double unbal = (r1 - r2) / (r1 + r2) * 100.0;
                    int excelRow = 7 + (pOdd - 1);
                    xlsx.write(excelRow, 8, QString::number(unbal, 'f', 2), measDataFormat);
                }
            }
        }

    // --- 绘制底部区域 ---
    xlsx.write("A55", "对绞  单芯", groupLabelFormat);
    xlsx.mergeCells("A55:A70", groupLabelFormat);

    QString description =
        "       20℃时电缆长度为1000米的标准值:\n"
        "       导线的直流电阻值为23.5±1Ω,普通电缆绝缘电阻值不小于3000MΩ，电阻不平衡系数不大于2%，\n"
        "       四芯组线间工作电容值50nf/km，对绞组线间工作电容值为70nf/km；\n"
        "       单根芯线対连到地的其它绝缘芯线间电容不大于100nf/km；\n"
        "       数字电缆绝缘电阻值不小于10000MΩ,四芯组线间工作电容值28+2nf/km,对绞组线间工作电容值为35+4nf/km;\n"
        "       单根芯线对连到地的其他绝缘芯线间电容不大于70nf/km,电阻不平衡系数不大于1%;\n"
        "       本盘电缆经换算成长度为1000米时各项数值。";

    xlsx.write("A71", description, footerFormat);
    xlsx.mergeCells("A71:I74", footerFormat);
    xlsx.setRowHeight(71, 110);

    // 绘制最后一行签字位：两端分布
    xlsx.write(75, 1, QString("  技术负责人：%1").arg(info->techDirector()), signLeftFormat);
    xlsx.mergeCells("A75:E75"); // 左侧占 5 列
    xlsx.write(75, 6, QString("监理单位：%1  ").arg(info->superUnit()), signLeftFormat);
    xlsx.mergeCells("F75:I75"); // 右侧占 4 列
    xlsx.setRowHeight(75, 22);

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

void ControlPanelManager::state_change()
{
    // TODO: 定时轮询硬件状态，更新测量数据
    qDebug() << "定时器触发: 更新状态！";

    switch (measure_state)
    {   
        case INIT_DEVS:
            on_init_devs();                     //1.0 初始化串口
            break;
        case MAKE_RECORD_FILE:
            on_make_record_file();              //2.0 创建记录文件
            break;
        case READ_MEAS_CONFIG:
            on_read_meas_config();              //3.0 读取测量配置
            break;
        case CONFIG_C_MODULE:
            on_config_c_module();               //4.0 配置电容模块
            break;
        case CONFIG_C_MODULE_WAIT:
            on_config_c_module_wait();          //4.1 等待配置电容模块
            break;
        case TO_LOW_R_MODULE:
            on_to_low_r_module();               //4.2 将电阻模块切换至低阻模式
            break;
        case TO_LOW_R_MODULE_WAIT:
            on_to_low_r_module_wait();          //4.3 等待将电阻模块切换至低阻模式
            break;
        case TO_HIGH_R_MODULE:
            on_to_high_r_module();              //4.4 将电阻模块切换至高阻模式
            break;
        case TO_HIGH_R_MODULE_WAIT:
            on_to_high_r_module_wait();         //4.5 等待将电阻模块切换至高阻模式
            break;
        case R_MODULE_OFF:    
            on_r_module_off();                  //4.6 将电阻模块测量关闭
            break;
        case R_MODULE_OFF_WAIT:
            on_r_module_off_wait();             //4.7 等待将电阻模块测量关闭
            break;
        case MODULE_SELECTION_TO_LOW_R:     
            on_module_selection_to_low_r();     //5.0 将模式选择模块切换至低电阻模式
            break;
        case MODULE_SELECTION_TO_LOW_R_WAIT:
            on_module_selection_to_low_r_wait();//5.1 等待将模式选择模块切换至低电阻模式
            break;
        case MODULE_SELECTION_TO_HIGH_R:     
            on_module_selection_to_high_r();     //5.2 将模式选择模块切换至高电阻模式
            break;
        case MODULE_SELECTION_TO_HIGH_R_WAIT:
            on_module_selection_to_high_r_wait();//5.3 等待将模式选择模块切换至高电阻模式
            break;
        case MODULE_SELECTION_TO_C:
            on_module_selection_to_c();          //5.4 将模式选择模块切换至电容模式
            break;
        case MODULE_SELECTION_TO_C_WAIT:
            on_module_selection_to_c_wait();     //5.5 等待将模式选择模块切换至电容模式
            break;
        case CHANGE_SWITCH:
            if (m_measuringDC)
                on_change_switch();              //6.0 切换继电器状态 (直流-同步1:1)
            else if(m_measuringIns)
                on_change_switch_ins();          //6.0 切换继电器状态 (绝缘-嵌套全扫描)
            else if (m_measuringCap)
                on_change_switch_cap();          //6.0 切换继电器状态 (电容)
            break;
        case CHANGE_SWITCH_WAIT:
            on_change_switch_wait();            //6.1 等待继电器状态切换
            break;
        case TO_HIGH_R_MEAS:
            on_to_high_r_meas();                //7.0 开启高阻测量模式
            break;
        case TO_HIGH_R_MEAS_WAIT:
            on_to_high_r_meas_wait();           //7.1 等待开启高阻测量模式
            break;
        case INS_CHARGE_DEADTIME:
            on_ins_charge_deadtime();           //7.15 绝缘充电死区等待
            break;
        case MEASURE_R:
            on_measure_r();                     //7.2 采集电阻状态
            break;
        case MEASURE_C:
            on_measure_c();                     //7.3 采集电容状态
            break;
        case MEASURE_C_WAIT:
            on_measure_c_wait();                //7.4 等待电容采集
            break;
        case RECORD_DATA:
            on_record_data();                   //8.0 记录数据状态
            break;
        case CHANGE_SWITCH_UPDATE:
            if (m_measuringDC)
                on_change_switch_update_sync();  //9.0 直流模式：1:1同步推进
            else if (m_measuringIns)
                on_change_switch_update_ins();   //9.0 绝缘模式：嵌套全扫描
            else if (m_measuringCap)
                on_change_switch_update_cap();   //9.0 电容模式：电容模式更新函数
            break;
        case END_MEAS:
            on_end_meas();                      //10.0 结束测量状态
            break;
        case ERROR:
            qDebug() << "ERROR: 状态机处于错误状态，等待手动复位";
            break;
    }
    emit relayStateChanged();  // 每次状态轮询后通知QML刷新指示灯
}

void ControlPanelManager::on_init_devs()                          //1.0初始化串口
{
    qDebug() << "on_init_devs: " << "1.0 初始化串口";

    bool modbusOk = false;
    bool serialOk = false;

    // --- 1. 处理 Modbus RTU 客户端 ---
    if (!modbusDevice)
    {
        modbusDevice = new QModbusRtuSerialClient(this);
        connect(modbusDevice, &QModbusClient::stateChanged, this, [](QModbusDevice::State state) {
            qDebug() << "Modbus 状态变化:" << state;
        });
    }

    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        qDebug() << "on_init_devs: 配置Modbus →"
                 << "端口:" << MODBUS_PORT_NAME
                 << "波特率:" << MODBUS_BAUD_RATE
                 << "数据位:" << MODBUS_DATA_BITS
                 << "停止位:" << MODBUS_STOP_BITS
                 << "校验:" << MODBUS_PARITY
                 << "超时:" << (MACHINE_STATE_UPDATE_TIME - 10) << "ms";

        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, MODBUS_PORT_NAME);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, MODBUS_PARITY);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, MODBUS_BAUD_RATE);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, MODBUS_DATA_BITS);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, MODBUS_STOP_BITS);
        modbusDevice->setTimeout(MACHINE_STATE_UPDATE_TIME - 10);
        modbusDevice->setInterFrameDelay(MODBUS_INTER_FRAME_DELAY);
        modbusDevice->setNumberOfRetries(0);

        if (modbusDevice->connectDevice())
        {
            qDebug() << "on_init_devs: Modbus(" << MODBUS_PORT_NAME << ") 连接请求已发送";
            modbusOk = true;
        }
        else
        {
            qDebug() << "on_init_devs: Modbus(" << MODBUS_PORT_NAME << ") 连接失败:" << modbusDevice->errorString();
        }
    }
    else
    {
        modbusOk = true;
    }

    // --- 2. 处理标准串口 ---
    if (!m_comPort)
    {
        m_comPort = new QSerialPort(this);
        connect(m_comPort, &QSerialPort::readyRead, this, &ControlPanelManager::onSerialReadyRead);
    }

    if (!m_comPort->isOpen())
    {
        m_comPort->setPortName(SERIAL_PORT_NAME);
        m_comPort->setBaudRate(SERIAL_BAUD_RATE);
        m_comPort->setDataBits(SERIAL_DATA_BITS);
        m_comPort->setStopBits(SERIAL_STOP_BITS);
        m_comPort->setParity(SERIAL_PARITY);

        if (m_comPort->open(QIODevice::ReadWrite))
        {
            qDebug() << "on_init_devs: " << SERIAL_PORT_NAME << " 串口已成功打开";
            serialOk = true;
        }
        else
        {
            qDebug() << "on_init_devs: " << SERIAL_PORT_NAME << " 串口打开失败:" << m_comPort->errorString();
        }
    }
    else
    {
        serialOk = true;
    }

    // --- 3. 综合判断 ---
    if (modbusOk && serialOk)
    {
        qDebug() << "on_init_devs: 所有设备初始化成功，准备进入测量模式";
        measure_state = READ_MEAS_CONFIG;
    }
    else
    {
        QString errorMsg = "初始化未完成: ";
        if (!modbusOk) errorMsg += "[Modbus " MODBUS_PORT_NAME " 失败] ";
        if (!serialOk) errorMsg += "[串口 " SERIAL_PORT_NAME " 失败]";
        qDebug() << "on_init_devs: " << errorMsg;
        measure_state = ERROR;
    }
}

void ControlPanelManager::on_make_record_file()                   //2.0创建记录文件
{
    // TODO: 创建记录文件
    qDebug() << "on_make_record_file: " << "2.0 创建记录文件";

    measure_state = READ_MEAS_CONFIG;
}

void ControlPanelManager::on_read_meas_config()                   //3.0读取测量配置，决定下一测量目标
{
    qDebug() << "on_read_meas_config: " << "3.0 读取测量配置";

    if (m_testConfig)
    {
        qDebug() << "  直流电阻:" << m_testConfig->checkDC()  << "| 已测:" << m_dcMeasured;
        qDebug() << "  绝缘电阻:" << m_testConfig->checkIns() << "| 已测:" << m_insMeasured;
        qDebug() << "  工作电容:" << m_testConfig->checkCap() << "| 已测:" << m_capMeasured;

        // 优先级: 直流电阻 > 绝缘电阻 > 工作电容
        if (m_testConfig->checkDC() && !m_dcMeasured)
        {
            qDebug() << "  → 下一状态: TO_LOW_R_MODULE (4.2 直流电阻测量)";
            setTimerInterval(DC_STATE_UPDATE_TIME);
            m_measuringDC = true;
            m_syncMode = true;
            measure_state = TO_LOW_R_MODULE;
        }
        else if (m_testConfig->checkIns() && !m_insMeasured)
        {
            qDebug() << "  → 下一状态: MODULE_SELECTION_TO_HIGH_R (绝缘电阻测量入口)";
            setTimerInterval(INS_STATE_UPDATE_TIME);
            m_measuringDC = false;
            m_measuringIns = true;
            m_syncMode = false;
            measure_state = MODULE_SELECTION_TO_HIGH_R;
        }
        else if (m_testConfig->checkCap() && !m_capMeasured)
        {
            qDebug() << "  → 下一状态: MODULE_SELECTION_TO_C (电容测量)";
            setTimerInterval(CAP_STATE_UPDATE_TIME);
            m_measuringDC = false;
            m_measuringIns = false;
            m_measuringCap = true;
            m_syncMode = false;
            m_capPairIndex = 0;  // 从第一对开始
            measure_state = CONFIG_C_MODULE;
        }
        else
        {
            qDebug() << "  → 所有项目已测完或无项目选中，进入 END_MEAS";
            measure_state = END_MEAS;
        }
    }
    else
    {
        qDebug() << "  警告: testConfig 未设置!";
        measure_state = ERROR;
    }
}

// --- Modbus 写请求封装 ---
QModbusDataUnit ControlPanelManager::writeRequest_R_module() const
{
    const auto table = QModbusDataUnit::HoldingRegisters;
    int startAddress = 0x45;
    quint16 numberOfEntries = 0x01;
    return QModbusDataUnit(table, startAddress, numberOfEntries);
}

QModbusDataUnit ControlPanelManager::writeRequest_switch_module() const
{
    const auto table = QModbusDataUnit::HoldingRegisters;
    int startAddress = 0x00;
    quint16 numberOfEntries = 0x01;
    return QModbusDataUnit(table, startAddress, numberOfEntries);
}

//! [read_data_0]
QModbusDataUnit ControlPanelManager::readRequest_R_module() const
{
    const auto table = QModbusDataUnit::InputRegisters; //读取要读的寄存器类型

    int startAddress = 0x00;    //读取起始地址

    quint16 numberOfEntries = 0x03;

    return QModbusDataUnit(table, startAddress, numberOfEntries);
}
//! [read_data_0]

// ========== MODBUS继电器写入辅助函数 ==========
void ControlPanelManager::writeRelayBoard(quint16 boardAddr, quint16 value,
                                           quint16 &stateVar, bool &flagVar,
                                           const QString &label)
{
    if (!modbusDevice) {
        qDebug() << "writeRelayBoard: modbusDevice 为空!";
        measure_state = ERROR;
        return;
    }
    QModbusDataUnit writeUnit = writeRequest_switch_module();
    writeUnit.setValue(0, value);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, boardAddr))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this,
                    [this, reply, value, &stateVar, &flagVar, label]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_change_switch:" << tr("Write response error: %1 (Modbus exception: 0x%2)")
                                .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            flagVar = false;
                            measure_state = CHANGE_SWITCH;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_change_switch:" << tr("Write response error: %1 (code: 0x%2)")
                                .arg(reply->errorString()).arg(error, -1, 16);
                            flagVar = false;
                            measure_state = CHANGE_SWITCH;
                        }
                        else
                        {
                            stateVar = value;
                            qDebug() << "on_change_switch:" << label << "切换成功！ 状态:"
                                     << QString::number(stateVar, 2).rightJustified(16, '0');
                            flagVar = true;
                        }
                        reply->deleteLater();
                    });
        }
        else { reply->deleteLater(); }
    }
    else
    {
        qDebug() << "on_change_switch:" << tr("Write error: %1").arg(modbusDevice->errorString());
        measure_state = CHANGE_SWITCH;
    }
}

void ControlPanelManager::on_config_c_module()                 //4.0 配置电容模块
{
    qDebug() << "on_config_c_module: " << "4.0 配置电容模块";

    m_serialConfigSucc = false;

    if (m_comPort && m_comPort->isOpen())
    {
        QByteArray cmd = QByteArrayLiteral("SDS-FREQ(10000)");
        m_comPort->write(cmd);
        qDebug() << "on_config_c_module: 已发送 →" << cmd;
    }
    else
    {
        qDebug() << "on_config_c_module: COM7 未打开，无法发送";
    }

    measure_state = CONFIG_C_MODULE_WAIT;
}

void ControlPanelManager::onSerialReadyRead()
{
    if (!m_comPort)
        return;

    QByteArray all = m_comPort->readAll();
    QString str = QString::fromLatin1(all);
    qDebug() << "onSerialReadyRead: COM7 收到 →" << str;

    // --- 配置阶段：收到 "OK" ---
    if (str == "OK")
    {
        m_serialConfigSucc = true;
        qDebug() << "onSerialReadyRead: 配置成功 (OK)";
        return;
    }

    // --- 电容测量阶段 ---
    if (measure_state == MEASURE_C || measure_state == MEASURE_C_WAIT)
    {
        static QRegularExpression reC("C=([^;]+)");
        auto matchC = reC.match(str);
        if (!matchC.hasMatch()) return;

        m_capAttemptCount++;
        if (m_capAttemptCount > CAP_MAX_ATTEMPTS)
        {
            if (m_capValidCount > 0)
            {
                m_capValue = m_capSum_F / m_capValidCount;
                qDebug() << "onSerialReadyRead: 采集满" << CAP_MAX_ATTEMPTS << "次 → 有效值" << m_capValidCount << "个"
                         << "| 平均值:" << (m_capValue * 1e9) << "nF";
            }
            else
            {
                m_capValue = -1.0;
                qDebug() << "onSerialReadyRead: 采集满" << CAP_MAX_ATTEMPTS << "次 → 全部OL，记录为OVLD";
            }
            m_capMeasureSucc = true;
            return;
        }

        QString rawC = matchC.captured(1).trimmed();

        if (rawC == "--0L-")
        {
            m_capOlCount++;
            qDebug() << "onSerialReadyRead: 低于最小量程, 连续OL:" << m_capOlCount
                     << "| 已采有效值:" << m_capValidCount;
            if (m_capOlCount >= CAP_OL_COUNT_MAX)
            {
                m_capMeasureSucc = true;
                // 低于最小量程 → 按最小量程下限记录，而非 OVLD
                m_capValue = (m_capValidCount > 0) ? (m_capSum_F / m_capValidCount)
                                                   : (CAP_MIN_RANGE_PF * 1e-12);
                qDebug() << "onSerialReadyRead: 连续" << CAP_OL_COUNT_MAX << "次低于最小量程，采集完成 → "
                         << (m_capValue > 0 ? QString::number(m_capValue * 1e9) + " nF" : "OVLD");
            }
        }
        else
        {
            m_capOlCount = 0;

            static QRegularExpression reVal("([0-9]*\\.?[0-9]+)\\s*([a-zA-Zμ一-龥]+)");
            QRegularExpressionMatch match_v = reVal.match(rawC);
            if (match_v.hasMatch())
            {
                double val = match_v.captured(1).toDouble();
                QString unit = match_v.captured(2).toLower();

                if (unit == "mf") val *= 1e-3;
                else if (unit == "uf" || unit == "μf") val *= 1e-6;
                else if (unit == "nf") val *= 1e-9;
                else if (unit == "pf") val *= 1e-12;

                double val_nF = val * 1e9;
                m_capValidCount++;
                m_capSum_F += val;  // 累加，用于取平均
                double avgNf = (m_capSum_F / m_capValidCount) * 1e9;

                qDebug() << "onSerialReadyRead: 本次" << val_nF << "nF | 累计" << m_capValidCount
                         << "次 | 当前均值" << avgNf << "nF";

                // 稳定性判断：以第一个有效值为基准，后续波动 ≤20%
                if (m_capBaselineNf < 0)
                {
                    m_capBaselineNf = val_nF;
                    m_capStableCount = 1;
                    qDebug() << "onSerialReadyRead: 基准值 →" << val_nF << "nF (稳定 1/" << CAP_STABLE_COUNT << ")";
                }
                else
                {
                    double diff = qAbs(val_nF - m_capBaselineNf) / m_capBaselineNf;
                    if (diff <= CAP_STABILITY_TOLERANCE)
                    {
                        m_capStableCount++;
                        qDebug() << "onSerialReadyRead: 稳定" << m_capStableCount << "/" << CAP_STABLE_COUNT << " (偏离基准" << (diff * 100) << "%)";
                        if (m_capStableCount >= CAP_STABLE_COUNT)
                        {
                            m_capMeasureSucc = true;
                            m_capValue = m_capSum_F / m_capValidCount;
                            qDebug() << "onSerialReadyRead: 连续" << CAP_STABLE_COUNT << "次稳定(相对基准)，提前完成 → 均值" << (m_capValue * 1e9) << "nF";
                        }
                    }
                    else
                    {
                        m_capStableCount = 1;
                        m_capBaselineNf = val_nF;
                        qDebug() << "onSerialReadyRead: 偏离基准" << (diff * 100) << "% >" << (CAP_STABILITY_TOLERANCE * 100) << "%，重置基准 →" << val_nF << "nF";
                    }
                }
            }
        }

        if (m_capMeasureSucc)
        {
            qDebug() << "onSerialReadyRead: 最终电容值 (均值):" << m_capValue << "F ("
                     << (m_capValue * 1e9) << "nF)";
            m_capAttemptCount = 0;
            m_capOlCount = 0;
            m_capValidCount = 0;
            m_capStableCount = 0;
            m_capBaselineNf = -1.0;
            m_capSum_F = 0.0;
        }
    }
}

void ControlPanelManager::on_config_c_module_wait()            //4.1 等待配置电容模块
{
    qDebug() << "on_config_c_module_wait: " << "4.1 等待配置电容模块";

    if (m_serialConfigSucc)
    {
        qDebug() << "on_config_c_module_wait: COM7 配置成功响应已收到，进入下一状态";
        measure_state = MODULE_SELECTION_TO_C;
    }
    else
    {
        qDebug() << "on_config_c_module_wait: COM7 配置未成功，重新发送配置指令";
        measure_state = CONFIG_C_MODULE;
    }
}

void ControlPanelManager::on_to_low_r_module()                   //4.2将电阻模块切换至低阻模式
{
    // TODO: 4.将电阻模块切换至低阻模式
    qDebug() << "on_to_low_r_module: " << "4.2 将电阻模块切换至低阻模式";

    QModbusDataUnit writeUnit = writeRequest_R_module();
    writeUnit.setValue(0, 0x10);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, R_MODULE))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_to_r_module: " << tr("Write response error: %1 (Modbus exception: 0x%2)")
                            .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            measure_state = TO_LOW_R_MODULE;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_to_r_module: " << tr("Write response error: %1 (code: 0x%2)")
                            .arg(reply->errorString()).arg(error, -1, 16);
                            measure_state = TO_LOW_R_MODULE;
                        }
                        else if (error == QModbusDevice::NoError)
                        {
                            qDebug() << "on_to_r_module: " << "低阻模式转换成功";
                            measure_state = MODULE_SELECTION_TO_LOW_R;
                        }
                        reply->deleteLater();
                    });
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << "on_to_r_module: " << tr("Write error: %1").arg(modbusDevice->errorString());
    }

    measure_state = TO_LOW_R_MODULE_WAIT;
}

void ControlPanelManager::on_to_low_r_module_wait()              //4.3等待将电阻模块切换至低阻模式
{
    // TODO: 等待低阻模式切换完成
    qDebug() << "on_to_low_r_module_wait: " << "4.3 等待将电阻模块切换至低阻模式";

    measure_state = TO_LOW_R_MODULE;
}

void ControlPanelManager::on_to_high_r_module()                  //4.4将电阻模块切换至高阻模式
{
    qDebug() << "on_to_high_r_module: " << "4.4 将电阻模块切换至高阻模式";

    QModbusDataUnit writeUnit = writeRequest_R_module();
    writeUnit.setValue(0, 0x20);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, R_MODULE))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_to_r_module: " << tr("Write response error: %1 (Modbus exception: 0x%2)")
                            .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            measure_state = TO_HIGH_R_MODULE;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_to_r_module: " << tr("Write response error: %1 (code: 0x%2)")
                            .arg(reply->errorString()).arg(error, -1, 16);
                            measure_state = TO_HIGH_R_MODULE;
                        }
                        else if (error == QModbusDevice::NoError)
                        {
                            qDebug() << "on_to_r_module: " << "高阻模式转换成功";
                            measure_state = CHANGE_SWITCH;
                        }
                        reply->deleteLater();
                    });
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << "on_to_r_module: " << tr("Write error: %1").arg(modbusDevice->errorString());
    }

    measure_state = TO_HIGH_R_MODULE_WAIT;
}

void ControlPanelManager::on_to_high_r_module_wait()             //4.5等待将电阻模块切换至高阻模式
{
    // TODO: 等待高阻模式切换完成
    qDebug() << "on_to_high_r_module_wait: " << "4.5 等待将电阻模块切换至高阻模式";

    measure_state = TO_HIGH_R_MODULE;
}

void ControlPanelManager::on_r_module_off()                      //4.6 将电阻模块测量关闭
{
    qDebug() << "on_r_module_off: " << "4.6 将电阻模块测量关闭";

    QModbusDataUnit writeUnit = writeRequest_R_module();
    writeUnit.setValue(0, 0x00);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, R_MODULE))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_r_module_off: " << tr("Write response error: %1 (Modbus exception: 0x%2)")
                            .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            measure_state = R_MODULE_OFF;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_r_module_off: " << tr("Write response error: %1 (code: 0x%2)")
                            .arg(reply->errorString()).arg(error, -1, 16);
                            measure_state = R_MODULE_OFF;
                        }
                        else if (error == QModbusDevice::NoError)
                        {
                            qDebug() << "on_r_module_off: " << "电阻模块测量已关闭";
                            if (m_retestJustDone)
                                measure_state = END_MEAS;
                            else
                                measure_state = m_measuringIns ? CHANGE_SWITCH : READ_MEAS_CONFIG;
                        }
                        reply->deleteLater();
                    });
            measure_state = R_MODULE_OFF_WAIT;
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << "on_r_module_off: " << tr("Write error: %1").arg(modbusDevice->errorString());
    }
}

void ControlPanelManager::on_r_module_off_wait()                 //4.7 等待将电阻模块测量关闭
{
    qDebug() << "on_r_module_off_wait: " << "4.7 等待将电阻模块测量关闭";

    measure_state = R_MODULE_OFF;
}

void ControlPanelManager::on_module_selection_to_low_r()         //5.0将模式选择模块切换至低电阻模式
{
    qDebug() << "on_module_selection_to_low_r: " << "5.0 将模式选择模块切换至低电阻模式";

    QModbusDataUnit writeUnit = writeRequest_switch_module();
    writeUnit.setValue(0, R_MEAS_MODE_LOW);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, MODULE_SELECTION_SWITCH))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_module_selection_to_low_r: " << tr("Write response error: %1 (Modbus exception: 0x%2)")
                            .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            measure_state = MODULE_SELECTION_TO_LOW_R;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_module_selection_to_low_r: " << tr("Write response error: %1 (code: 0x%2)")
                            .arg(reply->errorString()).arg(error, -1, 16);
                            measure_state = MODULE_SELECTION_TO_LOW_R;
                        }
                        else if (error == QModbusDevice::NoError)
                        {
                            qDebug() << "on_module_selection_to_low_r: " << "模式选择模块切换至低电阻模式成功";
                            measure_state = CHANGE_SWITCH;
                        }
                        reply->deleteLater();
                    });
            measure_state = MODULE_SELECTION_TO_LOW_R_WAIT;
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << "on_module_selection_to_low_r: " << tr("Write error: %1").arg(modbusDevice->errorString());
    }
}

void ControlPanelManager::on_module_selection_to_low_r_wait()    //5.1等待将模式选择模块切换至电阻模式
{
    qDebug() << "on_module_selection_to_low_r_wait: " << "5.1 等待将模式选择模块切换至电阻模式";

    measure_state = MODULE_SELECTION_TO_LOW_R;
}

void ControlPanelManager::on_module_selection_to_high_r()         //5.2 将模式选择模块切换至高电阻模式
{
    qDebug() << "on_module_selection_to_high_r: " << "5.2 将模式选择模块切换至高电阻模式";

    QModbusDataUnit writeUnit = writeRequest_switch_module();
    writeUnit.setValue(0, R_MEAS_MODE_HIGH);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, MODULE_SELECTION_SWITCH))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_module_selection_to_high_r: " << tr("Write response error: %1 (Modbus exception: 0x%2)")
                            .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            measure_state = MODULE_SELECTION_TO_HIGH_R;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_module_selection_to_high_r: " << tr("Write response error: %1 (code: 0x%2)")
                            .arg(reply->errorString()).arg(error, -1, 16);
                            measure_state = MODULE_SELECTION_TO_HIGH_R;
                        }
                        else if (error == QModbusDevice::NoError)
                        {
                            qDebug() << "on_module_selection_to_high_r: " << "模式选择模块切换至高电阻模式成功";
                            measure_state = TO_HIGH_R_MODULE;
                        }
                        reply->deleteLater();
                    });
            measure_state = MODULE_SELECTION_TO_HIGH_R_WAIT;
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << "on_module_selection_to_high_r: " << tr("Write error: %1").arg(modbusDevice->errorString());
    }
}

void ControlPanelManager::on_module_selection_to_high_r_wait()    //5.3 等待将模式选择模块切换至高电阻模式
{
    qDebug() << "on_module_selection_to_high_r_wait: " << "5.3 等待将模式选择模块切换至高电阻模式";

    measure_state = MODULE_SELECTION_TO_HIGH_R;
}

void ControlPanelManager::on_module_selection_to_c()            //5.4 将模式选择模块切换至电容模式
{
    qDebug() << "on_module_selection_to_c: " << "5.4 将模式选择模块切换至电容模式";

    QModbusDataUnit writeUnit = writeRequest_switch_module();
    writeUnit.setValue(0, C_MEAS_MODE);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, MODULE_SELECTION_SWITCH))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_module_selection_to_c: " << tr("Write response error: %1 (Modbus exception: 0x%2)")
                            .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            measure_state = MODULE_SELECTION_TO_C;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_module_selection_to_c: " << tr("Write response error: %1 (code: 0x%2)")
                            .arg(reply->errorString()).arg(error, -1, 16);
                            measure_state = MODULE_SELECTION_TO_C;
                        }
                        else if (error == QModbusDevice::NoError)
                        {
                            qDebug() << "on_module_selection_to_c: " << "模式选择模块切换至电容模式成功";
                            measure_state = CHANGE_SWITCH;
                        }
                        reply->deleteLater();
                    });
            measure_state = MODULE_SELECTION_TO_C_WAIT;
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << "on_module_selection_to_c: " << tr("Write error: %1").arg(modbusDevice->errorString());
    }
}

void ControlPanelManager::on_module_selection_to_c_wait()       //5.5 等待将模式选择模块切换至电容模式
{
    qDebug() << "on_module_selection_to_c_wait: " << "5.5 等待将模式选择模块切换至电容模式";

    measure_state = MODULE_SELECTION_TO_C;
}

// ============ 6.0 继电器切换 — 直流电阻专用（同步1:1配对） ============
void ControlPanelManager::on_change_switch()
{
    qDebug() << "on_change_switch: " << "6. 切换继电器状态 (直流同步1:1)";

    if( switch_over == true )
    {
        m_dcMeasured = true;
        qDebug() << "直流电阻全部测量完毕, m_dcMeasured = true";
        switch_over = false;

        // 重测模式：直流测完 → 看是否还要测绝缘
        if (m_isRetestMode)
        {
            m_retestDoDC = false;
            if (m_retestDoIns)
            {
                m_retestIndex = 0;
                m_measuringDC = false;
                m_measuringIns = true;
                m_syncMode = false;
                switch_over = false;
                switch_once_over = false;
                is_stable = false;
                skip_r_flag = false;
                total_try_count = 0;
                consecutive_count = 0;
                last_range = -1;
                r_module_range = 0;
                m_insOvldRetry = 0;

                wire_left_selection_1_number = 0;
                wire_left_selection_2_number = 0;
                wire_left_selection_3_number = 0;
                wire_right_selection_1_number = 0;
                wire_right_selection_2_number = 0;
                wire_right_selection_3_number = 0;

                int firstPos = m_retestList[0];
                int nb = (firstPos - 1) / 16;
                int nw = (firstPos - 1) % 16;
                wire_left_selection_board = nb;
                wire_right_selection_board = nb;
                if (nb == 0) { wire_left_selection_1_number = nw; wire_right_selection_1_number = nw; }
                else if (nb == 1) { wire_left_selection_2_number = nw; wire_right_selection_2_number = nw; }
                else { wire_left_selection_3_number = nw; wire_right_selection_3_number = nw; }

                qDebug() << "startRetest: 进入绝缘电阻阶段，首个位置" << firstPos;
                measure_state = MODULE_SELECTION_TO_HIGH_R;
            }
            else if (m_retestDoCap)
            {
                m_retestIndex = 0;
                m_measuringDC = false;
                m_measuringIns = false;
                m_measuringCap = true;
                m_syncMode = false;
                switch_over = false;
                switch_once_over = false;
                m_serialConfigSucc = false;
                m_capMeasureSucc = false;
                m_capAttemptCount = 0;
                m_capOlCount = 0;
                m_capValidCount = 0;
                m_capStableCount = 0;
                m_capBaselineNf = -1.0;
                m_capSum_F = 0.0;

                wire_left_selection_1_number = 0;
                wire_left_selection_2_number = 0;
                wire_left_selection_3_number = 0;
                wire_right_selection_1_number = 0;
                wire_right_selection_2_number = 0;
                wire_right_selection_3_number = 0;

                int firstPos = m_retestList[0];
                int nb = (firstPos - 1) / 16;
                int nw = (firstPos - 1) % 16;
                wire_left_selection_board = nb;
                wire_right_selection_board = nb;
                if (nb == 0) { wire_left_selection_1_number = nw; wire_right_selection_1_number = nw; }
                else if (nb == 1) { wire_left_selection_2_number = nw; wire_right_selection_2_number = nw; }
                else { wire_left_selection_3_number = nw; wire_right_selection_3_number = nw; }

                setTimerInterval(CAP_STATE_UPDATE_TIME);
                m_capPairIndex = (firstPos - 1) / 2;  // 从首个重测位置的 pair 开始
                qDebug() << "startRetest: 进入工作电容阶段(DC→Cap)，首个位置" << firstPos
                         << "Pair" << m_capPairIndex;
                measure_state = CONFIG_C_MODULE;
                return;
            }
            else
            {
                // 没有更多重测项目，结束
                m_isRetestMode = false;
                m_retestJustDone = true;
                m_retestList.clear();
                emit retestModeChanged();
                qDebug() << "startRetest: 全部重测完成";
                measure_state = R_MODULE_OFF;
            }
            return;
        }

        measure_state = READ_MEAS_CONFIG;

        wire_left_selection_board = 0;
        wire_left_selection_1_number = 0;
        wire_left_selection_2_number = 0;
        wire_left_selection_3_number = 0;
        wire_right_selection_board = 0;
        wire_right_selection_1_number = 0;
        wire_right_selection_2_number = 0;
        wire_right_selection_3_number = 0;

        qDebug() << "本轮测量完毕，选择器已复位，返回READ_MEAS_CONFIG";
        return;
    }

    if( wire_right_selection_board == 0 )
    {
        if( wire_right_selection_1_number == 0 )
        {
            if(!switch_once_over)
                wire_right_selection_3_flag = false;
            if(!wire_right_selection_3_flag)
                writeRelayBoard(WIRE_RIGHT_SELECTION_3, 0x0000, wire_right_selection_3_state, wire_right_selection_3_flag, "右-07");
        }

        if(!switch_once_over)
            wire_right_selection_1_flag = false;
        if(!wire_right_selection_1_flag)
            writeRelayBoard(WIRE_RIGHT_SELECTION_1, 0x0001 << wire_right_selection_1_number,
                            wire_right_selection_1_state, wire_right_selection_1_flag, "右-03");

        // 同步模式：右侧board=0切换之后，同时切左侧board=0
        if(!switch_once_over)
        {
            wire_left_selection_1_flag = false;
            if (wire_left_selection_1_number == 0)
                wire_left_selection_3_flag = false;
        }
        if (!wire_left_selection_3_flag)
            writeRelayBoard(WIRE_LEFT_SELECTION_3, 0x0000, wire_left_selection_3_state, wire_left_selection_3_flag, "左-08(同步清零)");
        if(!wire_left_selection_1_flag)
            writeRelayBoard(WIRE_LEFT_SELECTION_1, 0x0001 << wire_left_selection_1_number,
                            wire_left_selection_1_state, wire_left_selection_1_flag, "左-02(同步)");

        measure_state = CHANGE_SWITCH_WAIT;
    }
    else if( wire_right_selection_board == 1 )
    {
        if( wire_right_selection_2_number == 0 )
        {
            if(!switch_once_over)
                wire_right_selection_1_flag = false;
            if(!wire_right_selection_1_flag)
                writeRelayBoard(WIRE_RIGHT_SELECTION_1, 0x0000, wire_right_selection_1_state, wire_right_selection_1_flag, "右-03");
        }

        if(!switch_once_over)
            wire_right_selection_2_flag = false;
        if(!wire_right_selection_2_flag)
            writeRelayBoard(WIRE_RIGHT_SELECTION_2, 0x0001 << wire_right_selection_2_number,
                            wire_right_selection_2_state, wire_right_selection_2_flag, "右-05");

        // 同步模式：右侧board=1切换之后，同时切左侧board=1
        if(!switch_once_over)
        {
            wire_left_selection_2_flag = false;
            if (wire_left_selection_2_number == 0)
                wire_left_selection_1_flag = false;
        }
        if (!wire_left_selection_1_flag)
            writeRelayBoard(WIRE_LEFT_SELECTION_1, 0x0000, wire_left_selection_1_state, wire_left_selection_1_flag, "左-02(同步清零)");
        if(!wire_left_selection_2_flag)
            writeRelayBoard(WIRE_LEFT_SELECTION_2, 0x0001 << wire_left_selection_2_number,
                            wire_left_selection_2_state, wire_left_selection_2_flag, "左-06(同步)");

        measure_state = CHANGE_SWITCH_WAIT;
    }
    else if( wire_right_selection_board == 2 )
    {
        if( wire_right_selection_3_number == 0 )
        {
            if(!switch_once_over)
                wire_right_selection_2_flag = false;
            if(!wire_right_selection_2_flag)
                writeRelayBoard(WIRE_RIGHT_SELECTION_2, 0x0000, wire_right_selection_2_state, wire_right_selection_2_flag, "右-05");
        }

        if(!switch_once_over)
            wire_right_selection_3_flag = false;
        if(!wire_right_selection_3_flag)
            writeRelayBoard(WIRE_RIGHT_SELECTION_3, 0x0001 << wire_right_selection_3_number,
                            wire_right_selection_3_state, wire_right_selection_3_flag, "右-07");

        // 同步模式：右侧board=2切换之后，同时切左侧board=2
        if(!switch_once_over)
        {
            wire_left_selection_3_flag = false;
            if (wire_left_selection_3_number == 0)
                wire_left_selection_2_flag = false;
        }
        if (!wire_left_selection_2_flag)
            writeRelayBoard(WIRE_LEFT_SELECTION_2, 0x0000, wire_left_selection_2_state, wire_left_selection_2_flag, "左-06(同步清零)");
        if(!wire_left_selection_3_flag)
            writeRelayBoard(WIRE_LEFT_SELECTION_3, 0x0001 << wire_left_selection_3_number,
                            wire_left_selection_3_state, wire_left_selection_3_flag, "左-08(同步)");

        measure_state = CHANGE_SWITCH_WAIT;
    }

    switch_once_over = true;
}

// ============ 6.0' 继电器切换 — 绝缘电阻专用（左单线 ↔ 右全通减一） ============
void ControlPanelManager::on_change_switch_ins()
{
    qDebug() << "on_change_switch_ins: " << "6. 切换继电器状态 (绝缘-左单线右全通减一)";

    if( switch_over == true )
    {
        m_insMeasured = true;
        m_measuringIns = false;
        qDebug() << "绝缘电阻全部测量完毕, m_insMeasured = true";
        switch_over = false;

        // 重测模式：绝缘阶段完成 → 看是否还要测电容
        if (m_isRetestMode)
        {
            m_retestDoIns = false;
            if (m_retestDoCap)
            {
                m_retestIndex = 0;
                m_measuringDC = false;
                m_measuringIns = false;
                m_measuringCap = true;
                m_syncMode = false;
                switch_over = false;
                switch_once_over = false;
                m_serialConfigSucc = false;
                m_capMeasureSucc = false;
                m_capAttemptCount = 0;
                m_capOlCount = 0;
                m_capValidCount = 0;
                m_capStableCount = 0;
                m_capBaselineNf = -1.0;
                m_capSum_F = 0.0;

                wire_left_selection_1_number = 0;
                wire_left_selection_2_number = 0;
                wire_left_selection_3_number = 0;
                wire_right_selection_1_number = 0;
                wire_right_selection_2_number = 0;
                wire_right_selection_3_number = 0;

                int firstPos = m_retestList[0];
                int nb = (firstPos - 1) / 16;
                int nw = (firstPos - 1) % 16;
                wire_left_selection_board = nb;
                wire_right_selection_board = nb;
                if (nb == 0) { wire_left_selection_1_number = nw; wire_right_selection_1_number = nw; }
                else if (nb == 1) { wire_left_selection_2_number = nw; wire_right_selection_2_number = nw; }
                else { wire_left_selection_3_number = nw; wire_right_selection_3_number = nw; }

                setTimerInterval(CAP_STATE_UPDATE_TIME);
                m_capPairIndex = (firstPos - 1) / 2;  // 从首个重测位置的 pair 开始
                qDebug() << "startRetest: 进入工作电容阶段(Ins→Cap)，首个位置" << firstPos
                         << "Pair" << m_capPairIndex;
                measure_state = CONFIG_C_MODULE;
                return;
            }
            else
            {
                m_isRetestMode = false;
                m_retestJustDone = true;
                m_retestList.clear();
                emit retestModeChanged();
                qDebug() << "重测全部完成";
                measure_state = R_MODULE_OFF;
            }
        }
        else
        {
            measure_state = READ_MEAS_CONFIG;
        }

        wire_left_selection_board = 0;
        wire_left_selection_1_number = 0;
        wire_left_selection_2_number = 0;
        wire_left_selection_3_number = 0;
        wire_right_selection_board = 0;
        wire_right_selection_1_number = 0;
        wire_right_selection_2_number = 0;
        wire_right_selection_3_number = 0;

        qDebug() << "本轮测量完毕，选择器已复位";
        return;
    }

    int lb = wire_left_selection_board;
    int ln = (lb == 0) ? wire_left_selection_1_number
                     : (lb == 1) ? wire_left_selection_2_number
                                 : wire_left_selection_3_number;

    // 右侧板子数组
    quint16 rightAddrs[3] = {WIRE_RIGHT_SELECTION_1, WIRE_RIGHT_SELECTION_2, WIRE_RIGHT_SELECTION_3};
    quint16 *rightStates[3] = {&wire_right_selection_1_state, &wire_right_selection_2_state, &wire_right_selection_3_state};
    bool    *rightFlags[3]  = {&wire_right_selection_1_flag,  &wire_right_selection_2_flag,  &wire_right_selection_3_flag};

    // 左侧板子数组
    quint16 leftAddrs[3]  = {WIRE_LEFT_SELECTION_1, WIRE_LEFT_SELECTION_2, WIRE_LEFT_SELECTION_3};
    quint16 *leftStates[3] = {&wire_left_selection_1_state, &wire_left_selection_2_state, &wire_left_selection_3_state};
    bool    *leftFlags[3]  = {&wire_left_selection_1_flag,  &wire_left_selection_2_flag,  &wire_left_selection_3_flag};

    // ========== 右侧：全部板子全通，配对板排除对应线 ==========
    for (int b = 0; b < 3; b++) {
        if (b > group) break;
        // 每块板的全通掩码：前 group 块=0xFFFF，第 group 块=(1<<number)-1
        quint16 mask = (b < group) ? 0xFFFF : ((1 << number) - 1);
        if (b == lb) mask &= ~(1 << ln);   // 和左线配对的那根排除

        if (!switch_once_over) *rightFlags[b] = false;
        if (!*rightFlags[b])
            writeRelayBoard(rightAddrs[b], mask, *rightStates[b], *rightFlags[b],
                           QString("右-%1(绝缘)").arg(rightAddrs[b], 2, 16, QChar('0')));
    }

    // ========== 左侧：只接通当前左线，其余全关 ==========
    for (int b = 0; b < 3; b++) {
        if (b > group) break;
        quint16 mask = (b == lb) ? (quint16)(1 << ln) : 0x0000;

        if (!switch_once_over) *leftFlags[b] = false;
        if (!*leftFlags[b])
            writeRelayBoard(leftAddrs[b], mask, *leftStates[b], *leftFlags[b],
                           QString("左-%1(绝缘)").arg(leftAddrs[b], 2, 16, QChar('0')));
    }

    measure_state = CHANGE_SWITCH_WAIT;
    switch_once_over = true;
}

// ============ 6.0'' 继电器切换 — 工作电容专用（左右逐对同步） ============
void ControlPanelManager::on_change_switch_cap()
{
    qDebug() << "on_change_switch_cap: " << "6.0 切换继电器状态 (电容-逐对同步)";

    if( switch_over == true )
    {
        m_capMeasured = true;
        m_measuringCap = false;
        qDebug() << "工作电容全部测量完毕, m_capMeasured = true";
        switch_over = false;
        setTimerInterval(MACHINE_STATE_UPDATE_TIME);  // 恢复 100ms

        // 重测模式：电容阶段完成 → 真正结束
        if (m_isRetestMode)
        {
            m_retestDoCap = false;
            m_isRetestMode = false;
            m_retestJustDone = true;
            m_retestList.clear();
            emit retestModeChanged();
            qDebug() << "重测全部完成";
            measure_state = R_MODULE_OFF;
        }
        else
        {
            measure_state = READ_MEAS_CONFIG;
        }

        m_capPairIndex = 0;
        wire_left_selection_board = 0;
        wire_left_selection_1_number = 0;
        wire_left_selection_2_number = 0;
        wire_left_selection_3_number = 0;
        wire_right_selection_board = 0;
        wire_right_selection_1_number = 0;
        wire_right_selection_2_number = 0;
        wire_right_selection_3_number = 0;

        qDebug() << "本轮测量完毕，选择器已复位";
        return;
    }

    // 从 pair 索引反算左右线号 (0-based)
    int leftWire  = m_capPairIndex * 2;       // 偶: 0,2,4,...
    int rightWire = m_capPairIndex * 2 + 1;   // 奇: 1,3,5,...

    int board    = leftWire / 16;   // 左右必在同一板
    int leftNum  = leftWire % 16;
    int rightNum = rightWire % 16;

    qDebug() << "on_change_switch_cap: Pair" << m_capPairIndex
             << "→ 左:板" << board << "线" << leftNum
             << "右:板" << board << "线" << rightNum;

    // 右侧板子数组
    quint16 rightAddrs[3] = {WIRE_RIGHT_SELECTION_1, WIRE_RIGHT_SELECTION_2, WIRE_RIGHT_SELECTION_3};
    quint16 *rightStates[3] = {&wire_right_selection_1_state, &wire_right_selection_2_state, &wire_right_selection_3_state};
    bool    *rightFlags[3]  = {&wire_right_selection_1_flag,  &wire_right_selection_2_flag,  &wire_right_selection_3_flag};

    // 左侧板子数组
    quint16 leftAddrs[3]  = {WIRE_LEFT_SELECTION_1, WIRE_LEFT_SELECTION_2, WIRE_LEFT_SELECTION_3};
    quint16 *leftStates[3] = {&wire_left_selection_1_state, &wire_left_selection_2_state, &wire_left_selection_3_state};
    bool    *leftFlags[3]  = {&wire_left_selection_1_flag,  &wire_left_selection_2_flag,  &wire_left_selection_3_flag};

    // ========== 右侧：仅接通右线，其他板全断 ==========
    for (int b = 0; b < 3; b++) {
        if (b > group) break;
        quint16 mask = (b == board) ? (quint16)(1 << rightNum) : 0x0000;

        if (!switch_once_over) *rightFlags[b] = false;
        if (!*rightFlags[b])
            writeRelayBoard(rightAddrs[b], mask, *rightStates[b], *rightFlags[b],
                           QString("右-%1(电容)").arg(rightAddrs[b], 2, 16, QChar('0')));
    }

    // ========== 左侧：仅接通左线，其他板全断 ==========
    for (int b = 0; b < 3; b++) {
        if (b > group) break;
        quint16 mask = (b == board) ? (quint16)(1 << leftNum) : 0x0000;

        if (!switch_once_over) *leftFlags[b] = false;
        if (!*leftFlags[b])
            writeRelayBoard(leftAddrs[b], mask, *leftStates[b], *leftFlags[b],
                           QString("左-%1(电容)").arg(leftAddrs[b], 2, 16, QChar('0')));
    }

    // 超出 group 范围的板无需切换，直接标记完成
    for (int b = group + 1; b < 3; b++) {
        *rightFlags[b] = true;
        *leftFlags[b] = true;
    }

    measure_state = CHANGE_SWITCH_WAIT;
    switch_once_over = true;
}

void ControlPanelManager::on_change_switch_wait()                //6.1 等待继电器状态切换
{
    qDebug() << "on_change_switch_wait: " << "6.1 等待继电器状态切换";

    // qDebug() << "on_change_switch_wait: " << "wire_right_selection_1_flag: " << wire_right_selection_1_flag <<  "wire_right_selection_2_flag: " << wire_right_selection_2_flag <<  "wire_right_selection_3_flag: " << wire_right_selection_3_flag;

    if(wire_right_selection_1_flag && wire_right_selection_2_flag && wire_right_selection_3_flag && wire_left_selection_1_flag && wire_left_selection_2_flag && wire_left_selection_3_flag)
    {
        // 绝缘模式切换完成后直接进入充电死区（不再关闭/开启高阻测量），直流模式直接采集
        if (m_measuringCap)
        {
            m_capAttemptCount = 0;
            m_capOlCount = 0;
            m_capValidCount = 0;
            m_capStableCount = 0;
            m_capBaselineNf = -1.0;
            m_capSum_F = 0.0;
        }
        if (m_measuringIns)
            m_chargeTicks = 0;
        measure_state = m_measuringIns ? INS_CHARGE_DEADTIME
                      : m_measuringCap ? MEASURE_C
                      : MEASURE_R;
    }
    else
    {
        measure_state = CHANGE_SWITCH_WAIT;
    }
}

void ControlPanelManager::onReadReady()
{
    uint16_t data[3] = {0};

    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    qDebug() << "=== Modbus 回复开始 ===";

    if (reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit unit = reply->result();
        qDebug() << "onReadReady: 成功读取" << unit.valueCount() << "个寄存器;"
                 << "起始地址=0x" << QString::number(unit.startAddress(), 16)
                 << "寄存器类型=" << unit.registerType();

        for (qsizetype i = 0, total = unit.valueCount(); i < total; ++i)
            data[i] = unit.value(i);
        modebus_value = (data[0] << 16) + data[1];

        // DEBUG: 打印原始 Modbus 寄存器值
        qDebug() << "onReadReady: 原始寄存器"
                 << QString("data[0]=0x%1(%2)").arg(data[0], 4, 16, QChar('0')).arg(data[0])
                 << QString("data[1]=0x%1(%2)").arg(data[1], 4, 16, QChar('0')).arg(data[1])
                 << QString("data[2]=0x%1(%2)").arg(data[2], 4, 16, QChar('0')).arg(data[2])
                 << "→ modebus_value(raw)=" << modebus_value;

        if( (data[0] == 0x8000) && (data[1] == 0x8000))
        {
            qDebug() << "OVLD";
            r_module_range = -1;
        }
        else
        {
            // 挡位码 → (倍率, 除数, 挡位号) 查表
            struct RangeEntry { uint8_t code; double mul; double div; int range; };
            static const RangeEntry RANGES[] = {
                {0xa4, 0.001, 1.0, 0},   {0xa5, 0.01,  1.0, 1},
                {0xa6, 0.1,   1.0, 2},   {0xac, 0.001, 1.0, 3},
                {0xab, 0.01,  1.0, 4},   {0xaa, 0.1, 1000.0, 5},
                {0xa9, 1.0, 1000.0, 6},  {0xa8, 0.01,  1.0, 7},
                {0xa7, 0.1, 1000.0, 8},  {0x98, 1.0, 1000.0, 9},
                {0x99, 0.01, 1000.0,10}, {0x9a, 0.1, 1000.0,11},
                {0x9b, 1.0, 1000.0,12},  {0x9c, 0.01, 1000.0,13}, {0x9d, 0.1, 1000.0,14},
            };
            uint8_t code = data[2] & 0xff;
            bool found = false;
            for (const auto &r : RANGES) {
                if (r.code == code) {
                    modebus_value = modebus_value * r.mul / r.div;
                    r_module_range = r.range;
                    found = true;
                    break;
                }
            }
            if (!found) {
                qDebug() << "onReadReady: 未知挡位码" << Qt::hex << code;
                r_module_range = -1;
            }
            qDebug() << "电阻值：" << modebus_value << "挡位：" << r_module_range;
        }

        total_try_count++;

        // 情况 A：挡位是 -1 (特殊稳定状态)
        if (r_module_range == -1)
        {
            if (last_range == -1)
            {
                consecutive_count++;
            }
            else
            {
                // 刚切换到 -1 挡
                last_range = -1;
                consecutive_count = 1;
            }
            // 在 -1 挡位下，电阻值通常无效，不进行波动检查
        }
        // 情况 B：挡位是正常数值 (0, 1, 2...)
        else
        {
            // 如果是从其他挡位（如 -1 或不同挡位）切过来的
            if (r_module_range != last_range)
            {
                last_range = r_module_range;
                ref_value = modebus_value;
                consecutive_count = 1;
            }
            else
            {
                // 挡位相同，检查电阻值浮动：直流±10%，绝缘±15%
                float tolerance = m_measuringIns ? INS_STABILITY_TOLERANCE : DC_STABILITY_TOLERANCE;
                float lower_bound = ref_value * (1.0f - tolerance);
                float upper_bound = ref_value * (1.0f + tolerance);

                if (modebus_value >= lower_bound && modebus_value <= upper_bound)
                {
                    consecutive_count++;
                }
                else
                {
                    // 虽然挡位一样，但电阻跳变超过 10%
                    ref_value = modebus_value;
                    consecutive_count = 1;
                }
            }
        }

        // --- 计数判定 ---
        if (consecutive_count >= (m_measuringIns ? INS_HISTORY_SIZE : DC_HISTORY_SIZE))
        {
            is_stable = true;
            total_try_count = 0; // 判定成功，重置总计数器，为下次测量做准备
            consecutive_count = 0;
        }
        else if (total_try_count > (m_measuringIns ? INS_MAX_TRY_TIMES : DC_MAX_TRY_TIMES))
        {
            qDebug() << "警告：未稳定，强制跳过！";
            is_stable = true;    // 这里设为 true 是为了让流程走下去，或者你可以设一个额外的 skip 标志
            total_try_count = 0; // 重置计数器
            skip_r_flag = true;
            consecutive_count = 0;
        }
        else
        {
            is_stable = false;
        }
        qDebug() << "当前尝试次数：" << total_try_count << " 连续稳定次数：" << consecutive_count;
        qDebug() << "电阻测量稳定标志：" << is_stable;
    }
    else
    {
        qDebug() << "=== Modbus 回复失败 ==="
                 << "错误码:" << reply->error()
                 << "错误信息:" << reply->errorString();
    }
    qDebug() << "=== Modbus 回复结束 ===";
    reply->deleteLater();
}

void ControlPanelManager::on_to_high_r_meas()                   //7.0 开启高阻测量模式
{
    qDebug() << "on_to_high_r_meas: " << "7.0 开启高阻测量模式";

    QModbusDataUnit writeUnit = writeRequest_R_module();
    writeUnit.setValue(0, 0x20);

    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit, R_MODULE))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
                    {
                        const auto error = reply->error();
                        if (error == QModbusDevice::ProtocolError)
                        {
                            qDebug() << "on_to_high_r_meas: " << tr("Write response error: %1 (Modbus exception: 0x%2)")
                            .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                            measure_state = TO_HIGH_R_MEAS;
                        }
                        else if (error != QModbusDevice::NoError)
                        {
                            qDebug() << "on_to_high_r_meas: " << tr("Write response error: %1 (code: 0x%2)")
                            .arg(reply->errorString()).arg(error, -1, 16);
                            measure_state = TO_HIGH_R_MEAS;
                        }
                        else if (error == QModbusDevice::NoError)
                        {
                            qDebug() << "on_to_high_r_meas: " << "高阻测量模式已开启";
                            if (m_measuringIns) {
                                m_chargeTicks = 0;
                                measure_state = INS_CHARGE_DEADTIME;
                            } else {
                                measure_state = MEASURE_R;
                            }
                        }
                        reply->deleteLater();
                    });
            measure_state = TO_HIGH_R_MEAS_WAIT;
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << "on_to_high_r_meas: " << tr("Write error: %1").arg(modbusDevice->errorString());
    }
}

void ControlPanelManager::on_to_high_r_meas_wait()               //7.1 等待开启高阻测量模式
{
    qDebug() << "on_to_high_r_meas_wait: " << "7.1 等待开启高阻测量模式";

    measure_state = TO_HIGH_R_MEAS;
}

void ControlPanelManager::on_ins_charge_deadtime()              //7.15 绝缘充电死区等待
{
    int interval = m_timer ? m_timer->interval() : INS_STATE_UPDATE_TIME;
    m_chargeTicks += interval;

    if (m_chargeTicks >= INS_CHARGE_DEADTIME_MS)
    {
        qDebug() << "on_ins_charge_deadtime: 充电死区" << m_chargeTicks << "ms ≥"
                 << INS_CHARGE_DEADTIME_MS << "ms → 进入 MEASURE_R";
        m_chargeTicks = 0;
        measure_state = MEASURE_R;
    }
    else
    {
        // 继续等待，停留在当前状态
    }
}

void ControlPanelManager::on_measure_r()                         //7.0 采集电阻状态
{
    qDebug() << "on_measure_r: " << "7. 采集电阻状态"
             << "| 从站地址=" << R_MODULE
             << "| 起始寄存器=0x00 | 数量=3";

    if( !is_stable )
    {
        if (auto *reply = modbusDevice->sendReadRequest(readRequest_R_module(), R_MODULE))
        {
            qDebug() << "on_measure_r: Modbus读请求已发送 (InputRegisters, 地址0x00, 3个寄存器)";
            if (!reply->isFinished())
                connect(reply, &QModbusReply::finished, this, &ControlPanelManager::onReadReady);
            else
                delete reply;
        }
        else
        {
            qDebug() << "on_measure_r: " << tr("Read error: ") << modbusDevice->errorString();
        }
    }
    else
    {
        is_stable = false;
        measure_state = RECORD_DATA;
    }
}

void ControlPanelManager::on_measure_c()                         //7.0' 采集电容状态
{
    qDebug() << "on_measure_c: " << "7.0' 采集电容状态";

    m_capMeasureSucc = false;

    if (m_comPort && m_comPort->isOpen())
    {
        QByteArray cmd = QByteArrayLiteral("SDR-C");
        m_comPort->write(cmd);
        qDebug() << "on_measure_c: 已发送 SDR-C 指令";
    }
    else
    {
        // COM7 断开 → 超时计数器，防止死循环
        m_capAttemptCount++;
        qDebug() << "on_measure_c: COM7 未打开! 超时" << m_capAttemptCount << "/" << (CAP_MAX_ATTEMPTS * 2);
        if (m_capAttemptCount > CAP_MAX_ATTEMPTS * 2)
        {
            qDebug() << "on_measure_c: COM7 超时，强制结束 → OVLD";
            m_capValue = -1.0;
            m_capMeasureSucc = true;
        }
    }

    measure_state = MEASURE_C_WAIT;
}

void ControlPanelManager::on_measure_c_wait()                    //7.1 等待电容采集
{
    qDebug() << "on_measure_c_wait: " << "7.1 等待电容采集";

    if (m_capMeasureSucc)
    {
        qDebug() << "on_measure_c_wait: 电容采集完成 → 进入记录数据";
        measure_state = RECORD_DATA;
    }
    else
    {
        measure_state = MEASURE_C;
    }
}

void ControlPanelManager::on_record_data()                       //8.0 记录数据状态
{
    // Implementation for recording data
    qDebug() << "on_record_data: " << "8. 记录数据状态";
    double acct_r = 0;

    if(skip_r_flag) //判断是
    {
        skip_r_flag = false;
        qDebug() << "on_record_data: " << "跳过的测量";
        qDebug() << "on_record_data: " << "电阻值：" << modebus_value << "挡位：" << r_module_range;

    }

    if(r_module_range == -1)
    {
        modebus_value = -1;
    }
    qDebug() << "on_record_data: " << "电阻值：" << modebus_value << "挡位：" << r_module_range;

    // 挡位倍率查表: [0-2]=×0.001  [3-4]=×1  [5-7]=×1000  [8-9]=×1e6  [10-12]=×1e9  [13]=×1e12
    static const double RANGE_MULT[] = {0.001, 0.001, 0.001, 1.0, 1.0, 1000.0, 1000.0, 1000.0,
                                        1e6, 1e6, 1e9, 1e9, 1e9, 1e12, 1e13};
    acct_r = (r_module_range >= 0 && r_module_range <= 14)
                 ? modebus_value * RANGE_MULT[r_module_range]
                 : -1.0;

    // 直流电阻模式：记录数据
    if (m_measuringDC)
    {
        int wireNum = 0;
        if (wire_left_selection_board == 0)
            wireNum = wire_left_selection_1_number;
        else if (wire_left_selection_board == 1)
            wireNum = wire_left_selection_2_number;
        else if (wire_left_selection_board == 2)
            wireNum = wire_left_selection_3_number;
        int pos = wire_left_selection_board * 16 + wireNum + 1;  // +1从1开始

        if (r_module_range != -1)
        {
            // 有效值
            double dcConv = 0;
            if (m_cablePara && m_cablePara->cableLen() > 0)
                dcConv = acct_r / ( 1 + 0.003 * (5) ) * (1000.0 / m_cablePara->cableLen());
            qDebug() << "on_record_data: 直流电阻有效值 → 位置:" << pos
                     << "| 直阻:" << acct_r << "Ω"
                     << "| 换算值(1000m):" << dcConv << "Ω/km";
            emit newDcData(pos, acct_r, dcConv);
            m_measData[pos].dc = acct_r;
            m_measData[pos].dcConv = dcConv;
            m_measData[pos].hasDC = true;
            m_foundValidDc = true;
        }
        else if (m_syncMode)
        {
            // 同步模式下 OVLD 也要记录，用 -1 表示
            qDebug() << "on_record_data: 直流电阻 OVLD → 位置:" << pos;
            emit newDcData(pos, -1, -1);
            m_measData[pos].dc = -1.0;
            m_measData[pos].dcConv = -1.0;
            m_measData[pos].hasDC = true;
        }
    }
    else if (m_measuringIns)
    {
        int wireNum = 0;
        if (wire_left_selection_board == 0)
            wireNum = wire_left_selection_1_number;
        else if (wire_left_selection_board == 1)
            wireNum = wire_left_selection_2_number;
        else if (wire_left_selection_board == 2)
            wireNum = wire_left_selection_3_number;
        int pos = wire_left_selection_board * 16 + wireNum + 1;

        if (r_module_range != -1 && r_module_range < 10)
        {
            // 绝缘电阻有效值
            m_insOvldRetry = 0;
            double insVal = acct_r / 1000000.0;
            double insConv = 0;
            if (m_cablePara && m_cablePara->cableLen() > 0)
                insConv = insVal / (1 + 0.003 * 5) * (1000.0 / m_cablePara->cableLen());
            qDebug() << "on_record_data: 绝缘电阻有效值 → 位置:" << pos
                     << "| 线间:" << insVal << "MΩ"
                     << "| 换算值(1000m):" << insConv << "MΩ·km";
            emit newInsData(pos, insVal, insConv, insVal, insConv);
            m_measData[pos].insL1 = insVal;
            m_measData[pos].insL1Conv = insConv;
            m_measData[pos].insGND = insVal;
            m_measData[pos].insGNDConv = insConv;
            m_measData[pos].hasIns = true;
            measure_state = CHANGE_SWITCH_UPDATE;
        }
        else
        {
            // OVLD 或挡位过高 → 自动重试最多5次
            m_insOvldRetry++;
            if (m_insOvldRetry > INS_OVLD_RETRY_MAX)
            {
                qDebug() << "on_record_data: 绝缘OVLD重试" << INS_OVLD_RETRY_MAX << "次均失败 → 位置:" << pos;
                emit newInsData(pos, -1, -1, -1, -1);
                m_measData[pos].insL1 = -1.0;
                m_measData[pos].insL1Conv = -1.0;
                m_measData[pos].insGND = -1.0;
                m_measData[pos].insGNDConv = -1.0;
                m_measData[pos].hasIns = true;
                m_insOvldRetry = 0;
                measure_state = CHANGE_SWITCH_UPDATE;
            }
            else
            {
                qDebug() << "on_record_data: 绝缘OVLD，第" << m_insOvldRetry << "/" << INS_OVLD_RETRY_MAX << "次重试 → 位置:" << pos;
                // 跳过推进，同位置重新测量（高阻模块保持开启，直接重新切换继电器）
                measure_state = CHANGE_SWITCH;
            }
        }
    }
    else if (m_measuringCap)
    {
        // 逐对同步模式：从 pair 索引计算两个位置
        int posLeft  = m_capPairIndex * 2 + 1;   // 左线位置 (奇: 1,3,5,...)
        int posRight = posLeft + 1;               // 右线位置 (偶: 2,4,6,...)

        if (m_capValue > 0)
        {
            double cap_nF = m_capValue * 1e9;   // Farad → nF
            double capConv = 0;
            if (m_cablePara && m_cablePara->cableLen() > 0)
                capConv = cap_nF * (1000.0 / m_cablePara->cableLen());
            qDebug() << "on_record_data: 工作电容有效值 → Pair" << m_capPairIndex
                     << "| 位置:" << posLeft << "," << posRight
                     << "| 电容:" << cap_nF << "nF"
                     << "| 换算值(1000m):" << capConv << "nF/km";

            // 为该对的两个位置记录相同的电容值
            emit newCapData(posLeft, cap_nF, capConv);
            m_measData[posLeft].cap = cap_nF;
            m_measData[posLeft].capConv = capConv;
            m_measData[posLeft].hasCap = true;

            emit newCapData(posRight, cap_nF, capConv);
            m_measData[posRight].cap = cap_nF;
            m_measData[posRight].capConv = capConv;
            m_measData[posRight].hasCap = true;
        }
        else
        {
            qDebug() << "on_record_data: 工作电容 OVLD → Pair" << m_capPairIndex
                     << "| 位置:" << posLeft << "," << posRight;
            emit newCapData(posLeft, -1, -1);
            m_measData[posLeft].cap = -1.0;
            m_measData[posLeft].capConv = -1.0;
            m_measData[posLeft].hasCap = true;

            emit newCapData(posRight, -1, -1);
            m_measData[posRight].cap = -1.0;
            m_measData[posRight].capConv = -1.0;
            m_measData[posRight].hasCap = true;
        }
    }


    qDebug() << "on_record_data: " << "wire_left_selection_board: " << wire_left_selection_board << "wire_left_selection_1_number : " << wire_left_selection_1_number
                                                                                                 << "wire_left_selection_2_number : " << wire_left_selection_2_number
                                                                                                 << "wire_left_selection_3_number : " << wire_left_selection_3_number;

    qDebug() << "on_record_data: " << "wire_right_selection_board: " << wire_right_selection_board << "wire_right_selection_1_number : " << wire_right_selection_1_number
                                                                                                   << "wire_right_selection_2_number : " << wire_right_selection_2_number
                                                                                                   << "wire_right_selection_3_number : " << wire_right_selection_3_number;
    qDebug() << "on_record_data: " << "acct_r: " << acct_r ;

    if (measure_state != R_MODULE_OFF)
        measure_state = CHANGE_SWITCH_UPDATE;
    // 注: measure_state = R_MODULE_OFF 已在上面的else分支中设置
}

void ControlPanelManager::on_change_switch_update_ins()          //9.0 更新继电器状态 (绝缘-左单线推进)
{
    qDebug() << "on_change_switch_update_ins: " << "9. 更新继电器状态 (绝缘-左单线推进)";

    switch_once_over = false;

    // ============ 重测模式：按列表逐一重测绝缘 ============
    if (m_isRetestMode)
    {
        m_retestIndex++;
        if (m_retestIndex < m_retestList.size())
        {
            int pos = m_retestList[m_retestIndex];
            int nb = (pos - 1) / 16;
            int nw = (pos - 1) % 16;

            wire_left_selection_board = nb;
            wire_right_selection_board = nb;
            wire_left_selection_1_number = 0;
            wire_left_selection_2_number = 0;
            wire_left_selection_3_number = 0;
            wire_right_selection_1_number = 0;
            wire_right_selection_2_number = 0;
            wire_right_selection_3_number = 0;

            if (nb == 0) { wire_left_selection_1_number = nw; wire_right_selection_1_number = nw; }
            else if (nb == 1) { wire_left_selection_2_number = nw; wire_right_selection_2_number = nw; }
            else { wire_left_selection_3_number = nw; wire_right_selection_3_number = nw; }

            qDebug() << "on_change_switch_update_ins [重测]: 第" << m_retestIndex
                     << "/" << m_retestList.size() << "→ 位置" << pos;
        }
        else
        {
            switch_over = true;
            qDebug() << "on_change_switch_update_ins [重测]: 全部绝缘重测完成";
        }

        // 绝缘模式：直接进入下一轮切换（高阻模块始终保持开启，不再关闭）
        measure_state = CHANGE_SWITCH;
        return;
    }

    // ============ 正常绝缘模式 ============
    int totalCores = group * 16 + number;

    // 计算当前左侧索引 (0-based)
    int curIdx = wire_left_selection_board * 16;
    if (wire_left_selection_board == 0)
        curIdx = wire_left_selection_1_number;
    else if (wire_left_selection_board == 1)
        curIdx = 16 + wire_left_selection_2_number;
    else if (wire_left_selection_board == 2)
        curIdx = 32 + wire_left_selection_3_number;

    curIdx++;

    qDebug() << "on_change_switch_update_ins: 推进到第" << curIdx << "/" << totalCores << "根线";

    if (curIdx >= totalCores)
    {
        switch_over = true;
        qDebug() << "on_change_switch_update_ins: 全部" << totalCores << "根线已完成";
    }
    else
    {
        // 反算 board/wire，左右设同步位置
        int nb = curIdx / 16;
        int nw = curIdx % 16;

        wire_left_selection_board = nb;
        wire_right_selection_board = nb;

        // 清零所有 number，只给当前 board 设值
        wire_left_selection_1_number = 0;
        wire_left_selection_2_number = 0;
        wire_left_selection_3_number = 0;
        wire_right_selection_1_number = 0;
        wire_right_selection_2_number = 0;
        wire_right_selection_3_number = 0;

        if (nb == 0) {
            wire_left_selection_1_number = nw;
            wire_right_selection_1_number = nw;
        } else if (nb == 1) {
            wire_left_selection_2_number = nw;
            wire_right_selection_2_number = nw;
        } else if (nb == 2) {
            wire_left_selection_3_number = nw;
            wire_right_selection_3_number = nw;
        }

        qDebug() << "on_change_switch_update_ins: 左" << nb << "-" << nw
                 << "→ 右侧全通减一";
    }

    // 绝缘模式：直接进入下一轮切换（高阻模块始终保持开启，不再关闭）
    measure_state = CHANGE_SWITCH;
}

void ControlPanelManager::on_change_switch_update_cap()
{
    qDebug() << "on_change_switch_update_cap: " << "9. 更新继电器状态 (电容-逐对同步)";

    switch_once_over = false;
    int totalCores = group * 16 + number;

    // 信号电缆/数字信号电缆/内屏蔽数字信号电缆：仅四芯组测电容，对绞/单芯不测
    auto capGroups = [](const QString &type, int coreCount) -> int {
        if (type == "信号电缆") {
            switch (coreCount) {
            case 4:  return 1;   case 6:  case 8:  case 9:  return 0;
            case 12: case 14:    return 3;   case 16: case 19:    return 4;
            case 21:             return 4;   case 24:             return 5;
            case 28: case 30: case 33: case 37: case 42: case 44: return 7;
            case 48:             return 12;  default:              return coreCount / 4;
            }
        }
        if (type == "数字信号电缆") {
            switch (coreCount) {
            case 4:  return 1;   case 6:  return 0;   case 8:  case 9:  return 2;
            case 12: case 14:    return 3;   case 16: case 19:    return 4;
            case 21:             return 5;   case 24:             return 6;
            case 28: case 30: case 33: case 37: case 42: case 44: return 7;
            case 48:             return 12;  default:              return coreCount / 4;
            }
        }
        if (type == "内屏蔽数字信号电缆") {
            switch (coreCount) {
            case 8:  return 2;   case 12: case 14:    return 3;
            case 16: case 19:    return 4;   case 21:             return 5;
            case 24:             return 6;   case 28: case 30:    return 7;
            case 33:             return 8;   case 37:             return 9;
            case 42:             return 10;  case 44:             return 11;
            case 48:             return 12;  default:              return coreCount / 4;
            }
        }
        if (type == "其它电缆") {
            return (coreCount + 3) / 4; // 全部按四芯组计算，向上取整
        }
        return -1; // 其他类型不限
    };
    if (m_cablePara) {
        int grp = capGroups(m_cablePara->selectedCableType(), totalCores);
        if (grp >= 0) {
            int capCores = grp * 4;
            if (capCores < totalCores) {
                qDebug() << "on_change_switch_update_cap:"
                         << m_cablePara->selectedCableType()
                         << totalCores << "芯 → 仅四芯组" << capCores << "芯(" << grp << "组)测电容";
                totalCores = capCores;
            }
        }
    }

    int totalPairs = totalCores / 2;

    // ============ 重测模式：按列表逐一重测电容 ============
    if (m_isRetestMode)
    {
        m_retestIndex++;
        if (m_retestIndex < m_retestList.size())
        {
            int pos = m_retestList[m_retestIndex];
            // 重测按位置计算 pair 索引 (位置1→0, 位置2→0, 位置3→1...)
            m_capPairIndex = (pos - 1) / 2;
            qDebug() << "on_change_switch_update_cap [重测]: 第" << m_retestIndex
                     << "/" << m_retestList.size() << "→ 位置" << pos
                     << "Pair" << m_capPairIndex;
        }
        else
        {
            switch_over = true;
            qDebug() << "on_change_switch_update_cap [重测]: 全部电容重测完成";
        }

        measure_state = CHANGE_SWITCH;
        return;
    }

    // ============ 正常模式：推进到下一对 ============
    m_capPairIndex++;

    if (m_capPairIndex >= totalPairs)
    {
        switch_over = true;
        qDebug() << "on_change_switch_update_cap: 全部" << totalPairs << "对已完成";
    }
    else
    {
        qDebug() << "on_change_switch_update_cap: 推进到第" << m_capPairIndex << "/" << totalPairs << "对"
                 << "(线" << (m_capPairIndex * 2 + 1) << "-" << (m_capPairIndex * 2 + 2) << ")";
    }

    measure_state = CHANGE_SWITCH;
}

void ControlPanelManager::on_change_switch_update_sync()
{
    qDebug() << "on_change_switch_update_sync: " << "9. 更新继电器状态 (直流-1:1同步)";
    // ============ 同步模式：左右一一对应同时递增 ============
    // 左侧 board0/wire1 → 右侧 board0/wire1
    // 左侧 board0/wire2 → 右侧 board0/wire2
    // ... 同步递增，48芯电缆只需48步
    switch_once_over = false;

    // ============ 重测模式：按列表逐一重测 ============
    if (m_isRetestMode)
    {
        m_retestIndex++;
        if (m_retestIndex < m_retestList.size())
        {
            int pos = m_retestList[m_retestIndex];
            int newBoard = (pos - 1) / 16;
            int newNumber = (pos - 1) % 16;

            wire_left_selection_board = newBoard;
            wire_right_selection_board = newBoard;
            wire_left_selection_1_number = 0;
            wire_left_selection_2_number = 0;
            wire_left_selection_3_number = 0;
            wire_right_selection_1_number = 0;
            wire_right_selection_2_number = 0;
            wire_right_selection_3_number = 0;

            if (newBoard == 0)
            {
                wire_left_selection_1_number = newNumber;
                wire_right_selection_1_number = newNumber;
            }
            else if (newBoard == 1)
            {
                wire_left_selection_2_number = newNumber;
                wire_right_selection_2_number = newNumber;
            }
            else if (newBoard == 2)
            {
                wire_left_selection_3_number = newNumber;
                wire_right_selection_3_number = newNumber;
            }

            qDebug() << "on_change_switch_update_sync [重测]: 第" << m_retestIndex
                     << "/" << m_retestList.size() << "→ 位置" << pos
                     << "board" << newBoard << "wire" << newNumber;
        }
        else
        {
            // 直流重测列表完成，由 on_change_switch 判断是否进入绝缘阶段
            switch_over = true;
            qDebug() << "on_change_switch_update_sync [重测]: 直流重测列表完成";
        }

        measure_state = CHANGE_SWITCH;
        return;
    }

    // ============ 正常同步模式 ============
    int totalCores = group * 16 + number;  // 总芯数

    // 计算当前总索引 (0-based)
    int currentIndex = wire_right_selection_board * 16;
    if (wire_right_selection_board == 0)
        currentIndex = wire_right_selection_1_number;
    else if (wire_right_selection_board == 1)
        currentIndex = 16 + wire_right_selection_2_number;
    else if (wire_right_selection_board == 2)
        currentIndex = 32 + wire_right_selection_3_number;

    // 推进到下一对
    currentIndex++;

    qDebug() << "on_change_switch_update_sync: 推进到第" << currentIndex << "/" << totalCores << "对";

    if (currentIndex >= totalCores)
    {
        // 全部扫完
        switch_over = true;
        qDebug() << "on_change_switch_update_sync: 全部" << totalCores << "对已完成";
    }
    else
    {
        // 从索引反算 board 和 number，左右同步设置
        int newBoard = currentIndex / 16;
        int newNumber = currentIndex % 16;

        wire_left_selection_board = newBoard;
        wire_right_selection_board = newBoard;

        if (newBoard == 0)
        {
            wire_left_selection_1_number = newNumber;
            wire_right_selection_1_number = newNumber;
        }
        else if (newBoard == 1)
        {
            wire_left_selection_2_number = newNumber;
            wire_right_selection_2_number = newNumber;
        }
        else if (newBoard == 2)
        {
            wire_left_selection_3_number = newNumber;
            wire_right_selection_3_number = newNumber;
        }

        qDebug() << "on_change_switch_update_sync: 左" << newBoard << "-" << newNumber
                 << "↔ 右" << newBoard << "-" << newNumber;
    }

    measure_state = CHANGE_SWITCH;
}

void ControlPanelManager::on_end_meas()                          //10.0 结束测量状态
{
    qDebug() << "on_end_meas: " << "10.0 结束测量状态";

    bool wasRetest = m_retestJustDone;
    m_retestJustDone = false;

    setWorkState(IDLE);
    m_timer->stop();
    resetState();

    qDebug() << "测量完成：定时器已停止，状态已复位";

    if (wasRetest)
        emit retestCompleted();
    else
        emit measAllComplete();
}
