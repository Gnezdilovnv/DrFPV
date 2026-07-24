#ifndef SDRCONTROLLER_H
#define SDRCONTROLLER_H
#include <QObject>
#include <QVector>
#include <QThread>
#include <QMutex>
#include <complex>
struct SDRConfig {
    QString ip="192.168.2.1";
    double rxFrequency=2.4e9, txFrequency=2.4e9, sampleRate=5e6, rxBandwidth=18e6, txBandwidth=18e6;
    double rxGain1=30.0, rxGain2=30.0, txGain=-10.0;
    QString gainMode="manual";
    bool agcEnabled=false, dcOffsetTracking=true, quadratureTracking=true, firFiltersEnabled=true, enable2R2T=true, enableTX=false;
    int bufferSize=4096;
};
struct DeviceStatus { double rssi1=-100,rssi2=-100,temp=0,volt=0,cur=0; bool connected=false,streaming=false; };
class SDRController : public QObject {
    Q_OBJECT
public:
    explicit SDRController(QObject *parent=nullptr);
    ~SDRController();
    bool connectToDevice(const QString &ip="192.168.2.1");
    void disconnectDevice();
    bool configure(const SDRConfig &config);
    bool startStream();
    void stopStream();
    bool startTXStream();
    void stopTXStream();
    bool isConnected() const { return m_connected; }
    bool isStreaming() const { return m_streaming; }
    DeviceStatus getStatus() const { return m_status; }
    double getRSSI(int channel=0);
    double getTemperature();
    double getVoltage();
    double getCurrent();
    bool setFrequency(double freq, bool rx=true);
    bool setSampleRate(double rate);
    bool setBandwidth(double bw, bool rx=true);
    bool setGain(double gain, int channel=0);
    bool setGainMode(const QString &mode);
    bool setAGC(bool enabled);
    bool setDCOffsetTracking(bool enabled);
    bool setQuadratureTracking(bool enabled);
    bool setFIRFilters(bool enabled);
    bool set2R2T(bool enabled);
    bool runBIST();
    bool fastlockSave(const QString &name);
    bool fastlockLoad(const QString &name);
signals:
    void iqDataReady(const QVector<std::complex<float>> &data, int channel);
    void statusMessage(const QString &msg);
    void errorOccurred(const QString &error);
    void deviceStatusUpdated(const DeviceStatus &status);
private:
    void runStreamLoop();
    bool m_connected=false, m_streaming=false;
    SDRConfig m_config;
    DeviceStatus m_status;
    QThread *m_thread=nullptr;
    void *m_ctx=nullptr, *m_phy=nullptr, *m_rxDev=nullptr, *m_rxBuf=nullptr;
    QMutex m_mutex;
};
#endif
