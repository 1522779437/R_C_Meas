#ifndef CONTROLPANELMANAGER_H
#define CONTROLPANELMANAGER_H

#include <QObject>
#include <QTimer>
#include <QVariant>
#include <QModbusRtuSerialClient>
#include <QSerialPort>
#include <QModbusDataUnit>
#include "userinfomodel.h"
#include "testprojectconfig.h"
#include "cabletestpara.h"

#define MACHINE_STATE_UPDATE_TIME 60   // 默认定时器周期(ms) — Modbus超时等通用
#define DC_STATE_UPDATE_TIME      200   // 直流电阻测量定时器周期(ms)
#define INS_STATE_UPDATE_TIME     500   // 绝缘电阻测量定时器周期(ms)
#define CAP_STATE_UPDATE_TIME     600   // 工作电容测量定时器周期(ms)

// --- 直流电阻测量参数 ---
#define DC_HISTORY_SIZE           3     // DC连续稳定次数阈值
#define DC_MAX_TRY_TIMES          10    // DC最大采集尝试次数
#define DC_STABILITY_TOLERANCE    0.10f // DC电阻波动容差（±10%）

// --- 绝缘电阻测量参数 ---
#define INS_HISTORY_SIZE          3     // 绝缘连续稳定次数阈值
#define INS_MAX_TRY_TIMES         6    // 绝缘最大采集尝试次数
#define INS_STABILITY_TOLERANCE   0.15f // 绝缘电阻波动容差（±15%）
#define INS_OVLD_RETRY_MAX        3     // 绝缘OVLD最大重试次数
#define INS_CHARGE_DEADTIME_MS    1500  // 绝缘充电死区时间(ms)

// --- 电容测量参数 ---
#define CAP_MAX_ATTEMPTS          10    // 电容最大采集次数
#define CAP_OL_COUNT_MAX          5    // 电容连续OL判定阈值
#define CAP_STABLE_COUNT          3     // 电容连续稳定判定次数
#define CAP_STABILITY_TOLERANCE   0.20  // 电容稳定波动容差（±20%）
#define CAP_MIN_RANGE_PF          0.01  // 电容最小量程下限(pF) — 读数--0L-(低于最小量程)时按此值记录

// --- MODBUS串口配置 ---
#define MODBUS_PORT_NAME          "COM6"
// #define MODBUS_PORT_NAME          "/dev/ttyS7"

#define MODBUS_BAUD_RATE          QSerialPort::Baud115200
#define MODBUS_DATA_BITS          QSerialPort::Data8
#define MODBUS_STOP_BITS          QSerialPort::OneStop
#define MODBUS_PARITY             QSerialPort::NoParity
#define MODBUS_INTER_FRAME_DELAY  5000  // Modbus帧间延时(μs)

// --- 标准串口配置 ---
#define SERIAL_PORT_NAME          "COM7"
// #define SERIAL_PORT_NAME          "/dev/ttyS3"

#define SERIAL_BAUD_RATE          QSerialPort::Baud115200
#define SERIAL_DATA_BITS          QSerialPort::Data8
#define SERIAL_STOP_BITS          QSerialPort::OneStop
#define SERIAL_PARITY             QSerialPort::NoParity

#define R_MODULE                  0X01  // 电阻模块 Modbus 从机地址
#define MODULE_SELECTION_SWITCH   0X04

#define WIRE_LEFT_SELECTION_1     0X02
#define WIRE_LEFT_SELECTION_2     0X06
#define WIRE_LEFT_SELECTION_3     0X08

#define WIRE_RIGHT_SELECTION_1    0X03
#define WIRE_RIGHT_SELECTION_2    0X05
#define WIRE_RIGHT_SELECTION_3    0X07

// 模式
#define R_MEAS_MODE_LOW           0x8080
#define C_MEAS_MODE               0x0101
#define R_MEAS_MODE_HIGH          0x0505


class ControlPanelManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isWorking READ isWorking NOTIFY isWorkingChanged FINAL)
    Q_PROPERTY(int workState READ workState NOTIFY workStateChanged FINAL)
    Q_PROPERTY(quint16 wireLeftSelection1State READ wireLeftSelection1State NOTIFY relayStateChanged FINAL)
    Q_PROPERTY(quint16 wireLeftSelection2State READ wireLeftSelection2State NOTIFY relayStateChanged FINAL)
    Q_PROPERTY(quint16 wireLeftSelection3State READ wireLeftSelection3State NOTIFY relayStateChanged FINAL)
    Q_PROPERTY(quint16 wireRightSelection1State READ wireRightSelection1State NOTIFY relayStateChanged FINAL)
    Q_PROPERTY(quint16 wireRightSelection2State READ wireRightSelection2State NOTIFY relayStateChanged FINAL)
    Q_PROPERTY(quint16 wireRightSelection3State READ wireRightSelection3State NOTIFY relayStateChanged FINAL)
    Q_PROPERTY(bool isRetestMode READ isRetestMode NOTIFY retestModeChanged FINAL)

public:
    enum WorkState
    {
        IDLE = 0,    // 空闲
        WORKING = 1, // 工作中
        PAUSED = 2   // 已暂停
    };
    Q_ENUM(WorkState)

    explicit ControlPanelManager(QObject *parent = nullptr);
    ~ControlPanelManager();

    bool isWorking() const;
    int workState() const;
    void setWorkState(WorkState newState);

    quint16 wireLeftSelection1State() const { return wire_left_selection_1_state; }
    quint16 wireLeftSelection2State() const { return wire_left_selection_2_state; }
    quint16 wireLeftSelection3State() const { return wire_left_selection_3_state; }
    quint16 wireRightSelection1State() const { return wire_right_selection_1_state; }
    quint16 wireRightSelection2State() const { return wire_right_selection_2_state; }
    quint16 wireRightSelection3State() const { return wire_right_selection_3_state; }
    bool isRetestMode() const { return m_isRetestMode; }

    // --- 接口 1: 处理开始/停止逻辑 ---
    Q_INVOKABLE void processStartStop();

    // --- 接口 2: 处理结束逻辑 (强制复位/保存) ---
    Q_INVOKABLE void processEnd();

    // --- 接口 3: 处理导出逻辑 ---
    Q_INVOKABLE void processExport(QObject* infoObj);

    // --- 接口 4: 重测选中行 ---
    Q_INVOKABLE void startRetest(QVariantList positions);

    void setTestConfig(TestProjectConfig* config);
    void setCablePara(CableTestPara* para);

    enum MeasureState
    {
        MAKE_RECORD_FILE = 0,
        INIT_DEVS,
        READ_MEAS_CONFIG,
        CONFIG_C_MODULE,
        CONFIG_C_MODULE_WAIT,
        TO_LOW_R_MODULE,
        TO_LOW_R_MODULE_WAIT,
        TO_HIGH_R_MODULE,
        TO_HIGH_R_MODULE_WAIT,
        MODULE_SELECTION_TO_LOW_R ,
        MODULE_SELECTION_TO_LOW_R_WAIT ,
        MODULE_SELECTION_TO_HIGH_R ,
        MODULE_SELECTION_TO_HIGH_R_WAIT ,
        R_MODULE_OFF ,
        R_MODULE_OFF_WAIT ,
        MODULE_SELECTION_TO_C ,
        MODULE_SELECTION_TO_C_WAIT ,
        CHANGE_SWITCH ,
        CHANGE_SWITCH_WAIT ,
        TO_HIGH_R_MEAS ,
        TO_HIGH_R_MEAS_WAIT ,
        INS_CHARGE_DEADTIME ,            // 绝缘充电死区等待
        MEASURE_R ,
        MEASURE_C ,
        MEASURE_C_WAIT ,
        MEASURE_L ,
        MEASURE_L_WAIT ,
        RECORD_DATA ,
        CHANGE_SWITCH_UPDATE ,
        END_MEAS ,
        ERROR 
    };
    Q_ENUM(MeasureState)

signals:
    void isWorkingChanged();
    void workStateChanged();
    void exportFinished(bool success, QString filePath);
    void newDcData(int pos, double dc, double dcConv);  // 新增直流电阻数据
    void newInsData(int pos, double insL1, double insL1Conv, double insGND, double insGNDConv);  // 新增绝缘电阻数据
    void newCapData(int pos, double cap, double capConv);  // 新增工作电容数据
    void clearResults();  // 清空测试结果
    void measAllComplete();  // 所有测量项目完成
    void relayStateChanged(); // 继电器状态变更
    void retestCompleted();  // 重测完成
    void retestModeChanged(); // 重测模式变更

private:
    void on_make_record_file();
    void on_init_devs();
    void on_read_meas_config();
    void on_config_c_module();
    void on_config_c_module_wait();
    void on_to_low_r_module();
    void on_to_low_r_module_wait();
    void on_to_high_r_module();
    void on_to_high_r_module_wait();
    void on_module_selection_to_low_r();
    void on_module_selection_to_low_r_wait();
    void on_module_selection_to_high_r();
    void on_module_selection_to_high_r_wait();
    void on_module_selection_to_c();
    void on_module_selection_to_c_wait();
    void on_r_module_off();
    void on_r_module_off_wait();
    void on_change_switch();
    void on_change_switch_ins(); // 绝缘电阻模式：嵌套全扫描
    void on_change_switch_cap(); // 工作电容模式
    void on_change_switch_wait();
    void on_to_high_r_meas();
    void on_to_high_r_meas_wait();
    void on_ins_charge_deadtime();   // 绝缘充电死区等待
    void onReadReady();
    void on_measure_r();
    void on_measure_c();
    void on_measure_c_wait();
    void on_record_data();
    void on_change_switch_update_ins();    // 绝缘模式：嵌套全扫描
    void on_change_switch_update_sync();  // 直流模式：左右1:1同步递增
    void on_change_switch_update_cap();   // 电容模式
    void on_end_meas();

    QModbusDataUnit writeRequest_R_module() const;
    QModbusDataUnit writeRequest_switch_module() const;
    QModbusDataUnit readRequest_R_module() const;

    // MODBUS继电器写入辅助函数
    void writeRelayBoard(quint16 boardAddr, quint16 value,
                         quint16 &stateVar, bool &flagVar,
                         const QString &label);

    void resetState();
    void setTimerInterval(int ms);  // 动态切换定时器间隔

private slots:
    void state_change();
    void onSerialReadyRead();

private:
    WorkState m_workState = IDLE;
    QTimer* m_timer = nullptr;
    QModbusRtuSerialClient* modbusDevice = nullptr;
    QSerialPort* m_comPort = nullptr;   // COM7 标准串口
    TestProjectConfig* m_testConfig = nullptr;
    CableTestPara* m_cablePara = nullptr;
    MeasureState measure_state = MAKE_RECORD_FILE;
    bool m_dcMeasured = false;
    bool m_insMeasured = false;
    bool m_capMeasured = false;

    bool m_measuringDC = false;    // 当前是否在直流电阻测量模式
    bool m_measuringIns = false;   // 当前是否在绝缘电阻测量模式
    bool m_measuringCap = false;   // 当前是否在工作电容测量模式
    bool m_foundValidDc = false;   // 当前左线已找到有效直阻值，跳过剩余右线
    bool m_syncMode = false;       // true=左右同步1:1扫描, false=嵌套全扫描

    bool switch_over = false;
    //单次切换完成标志
    bool switch_once_over = false;

    // WIRE_LEFT_SELECTION_1继电器状态
    quint16 wire_left_selection_1_state = 0x0000;
    quint16 wire_left_selection_2_state = 0x0000;
    quint16 wire_left_selection_3_state = 0x0000;
    quint16 wire_left_selection_board = 0;
    quint16 wire_left_selection_1_number = 0x0000;
    quint16 wire_left_selection_2_number = 0x0000;
    quint16 wire_left_selection_3_number = 0x0000;
    bool wire_left_selection_1_flag = true;
    bool wire_left_selection_2_flag = true;
    bool wire_left_selection_3_flag = true;

    // WIRE_RIGHT_SELECTION_1继电器状态
    quint16 wire_right_selection_1_state = 0x0000;
    quint16 wire_right_selection_2_state = 0x0000;
    quint16 wire_right_selection_3_state = 0x0000;
    quint16 wire_right_selection_board = 0;
    quint16 wire_right_selection_1_number = 0x0000;
    quint16 wire_right_selection_2_number = 0x0000;
    quint16 wire_right_selection_3_number = 0x0000;
    bool wire_right_selection_1_flag = true;
    bool wire_right_selection_2_flag = true;
    bool wire_right_selection_3_flag = true;

    int total_try_count = 0;
    int last_range = -1;            // 上一次的挡位
    int consecutive_count = 0;      // 连续达标计数
    double ref_value = 0.0f;        // 基准电阻值
    bool is_stable = false;         // 最终标志位
    bool skip_r_flag = false;

    //测量的电阻值
    double modebus_value = 0;
    //电阻模块的挡位
    int r_module_range = 0;
    int group = 0;
    int number = 0;

    // 重测相关
    bool m_isRetestMode = false;
    bool m_retestJustDone = false;
    bool m_retestDoDC = false;     // 重测直流电阻
    bool m_retestDoIns = false;    // 重测绝缘电阻
    bool m_retestDoCap = false;   // 重测工作电容
    QList<int> m_retestList;
    int m_retestIndex = 0;
    int m_insOvldRetry = 0;        // 绝缘OVLD重试计数
    int m_chargeTicks = 0;         // 绝缘充电死区计时(ms)
    bool m_serialConfigSucc = false; // COM7 配置成功标志

    // COM7 电容解析相关
    int m_capPairIndex = 0;      // 电容配对索引 (0 ~ totalCores/2-1)，左右逐对同步
    int m_capAttemptCount = 0;
    int m_capOlCount = 0;
    int m_capValidCount = 0;      // 本次采集有效读数次数
    int m_capStableCount = 0;    // nF连续稳定计数
    double m_capBaselineNf = -1.0; // 第一个有效值(nF)，作为稳定性基准
    double m_capSum_F = 0.0;     // 有效值的累加和(F)，用于取平均
    double m_capValue = -1.0;
    bool m_capMeasureSucc = false;   // 电容测量完成标志

    // 测量结果缓存（供导出Excel用）
    struct MeasRow {
        double dc = -1.0;
        double dcConv = -1.0;
        double insL1 = -1.0;
        double insL1Conv = -1.0;
        double insGND = -1.0;
        double insGNDConv = -1.0;
        double cap = -1.0;
        double capConv = -1.0;
        bool hasDC = false;
        bool hasIns = false;
        bool hasCap = false;
    };
    QMap<int, MeasRow> m_measData;  // pos → 测量数据
};

#endif // CONTROLPANELMANAGER_H
