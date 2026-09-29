#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>
#include <QQmlContext>
#include "userinfomodel.h"
#include "cabletestpara.h"
#include "testprojectconfig.h"
#include "controlpanelmanager.h"

#ifdef Q_OS_LINUX
#include <signal.h>
#endif

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

#ifdef Q_OS_LINUX
    auto quitHandler = [](int) {
        QCoreApplication::quit();
    };
    signal(SIGINT, quitHandler);
    signal(SIGTERM, quitHandler);
#endif

    // QQuickStyle::setStyle("basic");
    QQuickStyle::setStyle("Material");


    UserInfoModel userModel;
    CableTestPara cablePara;
    TestProjectConfig testConfig;
    ControlPanelManager controlManager;
    controlManager.setTestConfig(&testConfig);
    controlManager.setCablePara(&cablePara);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("backendModel", &userModel);
    engine.rootContext()->setContextProperty("cableParaModel", &cablePara);
    engine.rootContext()->setContextProperty("testConfig", &testConfig);
    engine.rootContext()->setContextProperty("controlMgr", &controlManager);

    const QUrl url(u"qrc:/qt/qml/r_c_meas_v5/Main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
