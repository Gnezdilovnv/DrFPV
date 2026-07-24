#include "SDRController.h"
#include <cstring>
extern "C" {
#include <iio.h>
}
SDRController::SDRController(QObject *parent) : QObject(parent) {}
SDRController::~SDRController() { stopStream(); disconnectDevice(); }
bool SDRController::connectToDevice(const QString &ip) {
    QMutexLocker locker(&m_mutex);
    if(m_ctx) disconnectDevice();
    QString uri="ip:"+ip;
    m_ctx=iio_create_context_from_uri(uri.toStdString().c_str());
    if(!m_ctx){ emit errorOccurred("Не удалось подключиться"); return false; }
    m_connected=true; m_status.connected=true;
    emit statusMessage("Подключено к "+ip);
    emit deviceStatusUpdated(m_status);
    return true;
}
void SDRController::disconnectDevice() {
    QMutexLocker locker(&m_mutex);
    stopStream();
    if(m_ctx){ iio_context_destroy((struct iio_context*)m_ctx); m_ctx=nullptr; }
    m_connected=false; m_phy=nullptr; m_rxDev=nullptr;
    m_status.connected=false; m_status.streaming=false;
    emit deviceStatusUpdated(m_status);
    emit statusMessage("Отключено");
}
bool SDRController::configure(const SDRConfig &config) {
    QMutexLocker locker(&m_mutex);
    if(!m_ctx) return false;
    m_config=config;
    m_phy=iio_context_find_device((struct iio_context*)m_ctx,"ad9361-phy");
    if(!m_phy){ emit errorOccurred("AD9361 PHY не найден"); return false; }
    auto setLO=[&](const char *name, double freq){ auto ch=iio_device_find_channel((struct iio_device*)m_phy,name,true); if(ch) iio_channel_attr_write_longlong(ch,"frequency",(long long)freq); };
    setLO("altvoltage0",config.rxFrequency);
    setLO("altvoltage1",config.txFrequency);
    auto setRx=[&](const char *attr, long long val){ auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false); if(ch) iio_channel_attr_write_longlong(ch,attr,val); };
    setRx("sampling_frequency",(long long)config.sampleRate);
    setRx("rf_bandwidth",(long long)config.rxBandwidth);
    auto setGain=[&](const char *name, double gain){ auto ch=iio_device_find_channel((struct iio_device*)m_phy,name,false); if(ch){ char buf[16]; snprintf(buf,sizeof(buf),"%.1f",gain); iio_channel_attr_write(ch,"gain",buf); } };
    setGain("voltage0",config.rxGain1);
    setGain("voltage2",config.rxGain2);
    auto setMode=[&](const QString &mode){ auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false); if(ch) iio_channel_attr_write(ch,"gain_control_mode",mode.toStdString().c_str()); };
    setMode(config.gainMode);
    auto setBool=[&](const char *attr, bool val){ auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false); if(ch) iio_channel_attr_write(ch,attr,val?"1":"0"); };
    setBool("dc_offset_tracking",config.dcOffsetTracking);
    setBool("quadrature_tracking",config.quadratureTracking);
    setBool("filter_fir_enable",config.firFiltersEnabled);
    m_rxDev=iio_context_find_device((struct iio_context*)m_ctx,"cf-ad9361-lpc");
    if(!m_rxDev){ emit errorOccurred("Устройство захвата не найдено"); return false; }
    emit statusMessage("SDR настроен");
    return true;
}
bool SDRController::startStream() {
    QMutexLocker locker(&m_mutex);
    if(!m_rxDev || m_streaming) return false;
    auto enable=[&](const char *name){ auto ch=iio_device_find_channel((struct iio_device*)m_rxDev,name,false); if(ch) iio_channel_enable(ch); };
    enable("voltage0"); enable("voltage1");
    if(m_config.enable2R2T){ enable("voltage2"); enable("voltage3"); }
    m_rxBuf=iio_device_create_buffer((struct iio_device*)m_rxDev,m_config.bufferSize,false);
    if(!m_rxBuf){ emit errorOccurred("Не удалось создать буфер"); return false; }
    m_streaming=true; m_status.streaming=true;
    m_thread=QThread::create([this](){ runStreamLoop(); });
    m_thread->start();
    emit statusMessage("Поток запущен");
    emit deviceStatusUpdated(m_status);
    return true;
}
void SDRController::stopStream() {
    QMutexLocker locker(&m_mutex);
    m_streaming=false; m_status.streaming=false;
    if(m_thread){ m_thread->quit(); m_thread->wait(); delete m_thread; m_thread=nullptr; }
    if(m_rxBuf){ iio_buffer_destroy((struct iio_buffer*)m_rxBuf); m_rxBuf=nullptr; }
    emit statusMessage("Поток остановлен");
    emit deviceStatusUpdated(m_status);
}
void SDRController::runStreamLoop() {
    int numChannels=m_config.enable2R2T?2:1;
    int samplesPerChannel=m_config.bufferSize/(2*numChannels);
    QVector<std::complex<float>> rx1(samplesPerChannel), rx2(samplesPerChannel);
    while(m_streaming){
        if(iio_buffer_refill((struct iio_buffer*)m_rxBuf)<0){ emit errorOccurred("Ошибка refill"); break; }
        void *p_dat,*p_end; ptrdiff_t p_inc=iio_buffer_step((struct iio_buffer*)m_rxBuf);
        p_end=iio_buffer_end((struct iio_buffer*)m_rxBuf);
        int idx=0, i1=0, i2=0;
        for(p_dat=iio_buffer_first((struct iio_buffer*)m_rxBuf,nullptr); p_dat<p_end; p_dat=(char*)p_dat+p_inc){
            int16_t *s=(int16_t*)p_dat;
            int chIdx=idx%(2*numChannels);
            if(chIdx<2){ if(i1<samplesPerChannel){ if(chIdx==0) rx1[i1].real((float)s[0]/2048.0f); else { rx1[i1].imag((float)s[0]/2048.0f); i1++; } } }
            else { if(i2<samplesPerChannel){ if(chIdx==2) rx2[i2].real((float)s[0]/2048.0f); else { rx2[i2].imag((float)s[0]/2048.0f); i2++; } } }
            idx++;
        }
        emit iqDataReady(rx1,0);
        if(numChannels==2) emit iqDataReady(rx2,1);
    }
}
double SDRController::getRSSI(int channel) {
    if(!m_phy) return -100;
    const char *name=(channel==0)?"voltage0":"voltage2";
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,name,false);
    if(!ch) return -100;
    char buf[32]; iio_channel_attr_read(ch,"rssi",buf,sizeof(buf));
    double val=atof(buf);
    if(channel==0) m_status.rssi1=val; else m_status.rssi2=val;
    return val;
}
double SDRController::getTemperature() {
    if(!m_phy) return 0;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"temp0",false);
    if(!ch) return 0;
    char buf[32]; iio_channel_attr_read(ch,"input",buf,sizeof(buf));
    m_status.temp=atof(buf)/1000.0;
    return m_status.temp;
}
double SDRController::getVoltage() {
    if(!m_phy) return 0;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage4",false);
    if(!ch) return 0;
    char buf[32]; iio_channel_attr_read(ch,"input",buf,sizeof(buf));
    m_status.volt=atof(buf);
    return m_status.volt;
}
double SDRController::getCurrent() {
    if(!m_phy) return 0;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage5",false);
    if(!ch) return 0;
    char buf[32]; iio_channel_attr_read(ch,"input",buf,sizeof(buf));
    m_status.cur=atof(buf);
    return m_status.cur;
}
bool SDRController::setFrequency(double freq, bool rx) {
    if(!m_phy) return false;
    const char *name=rx?"altvoltage0":"altvoltage1";
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,name,true);
    if(!ch) return false;
    iio_channel_attr_write_longlong(ch,"frequency",(long long)freq);
    if(rx) m_config.rxFrequency=freq; else m_config.txFrequency=freq;
    return true;
}
bool SDRController::setSampleRate(double rate) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write_longlong(ch,"sampling_frequency",(long long)rate);
    m_config.sampleRate=rate;
    return true;
}
bool SDRController::setBandwidth(double bw, bool rx) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write_longlong(ch,"rf_bandwidth",(long long)bw);
    if(rx) m_config.rxBandwidth=bw; else m_config.txBandwidth=bw;
    return true;
}
bool SDRController::setGain(double gain, int channel) {
    if(!m_phy) return false;
    const char *name=(channel==0)?"voltage0":"voltage2";
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,name,false);
    if(!ch) return false;
    char buf[16]; snprintf(buf,sizeof(buf),"%.1f",gain);
    iio_channel_attr_write(ch,"gain",buf);
    if(channel==0) m_config.rxGain1=gain; else m_config.rxGain2=gain;
    return true;
}
bool SDRController::setGainMode(const QString &mode) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"gain_control_mode",mode.toStdString().c_str());
    m_config.gainMode=mode;
    return true;
}
bool SDRController::setAGC(bool enabled) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"gain_control_mode",enabled?"fast_attack":"manual");
    m_config.agcEnabled=enabled;
    return true;
}
bool SDRController::setDCOffsetTracking(bool enabled) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"dc_offset_tracking",enabled?"1":"0");
    m_config.dcOffsetTracking=enabled;
    return true;
}
bool SDRController::setQuadratureTracking(bool enabled) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"quadrature_tracking",enabled?"1":"0");
    m_config.quadratureTracking=enabled;
    return true;
}
bool SDRController::setFIRFilters(bool enabled) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"filter_fir_enable",enabled?"1":"0");
    m_config.firFiltersEnabled=enabled;
    return true;
}
bool SDRController::set2R2T(bool enabled) {
    m_config.enable2R2T=enabled;
    if(m_streaming){ stopStream(); startStream(); }
    return true;
}
bool SDRController::runBIST() {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"bist_tone","1");
    return true;
}
bool SDRController::fastlockSave(const QString &name) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"fastlock_save",name.toStdString().c_str());
    return true;
}
bool SDRController::fastlockLoad(const QString &name) {
    if(!m_phy) return false;
    auto ch=iio_device_find_channel((struct iio_device*)m_phy,"voltage0",false);
    if(!ch) return false;
    iio_channel_attr_write(ch,"fastlock_load",name.toStdString().c_str());
    return true;
}
