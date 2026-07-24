#include "SignalProcessor.h"
#include <cmath>
#include <algorithm>
#include <QDateTime>
SignalProcessor::SignalProcessor(QObject *parent) : QObject(parent) {}
void SignalProcessor::processIQData(const QVector<std::complex<float>> &data, double sampleRate, double centerFreq, int channel) {
    if(data.isEmpty()) return;
    QVector<double> magnitudes(data.size()/2);
    for(int i=0;i<data.size()/2;i++) magnitudes[i]=20*log10(std::abs(data[i])+1e-10);
    QVector<double> freqs(data.size()/2);
    double df=sampleRate/data.size();
    for(int i=0;i<freqs.size();i++) freqs[i]=centerFreq-sampleRate/2+i*df;
    emit spectrumReady(freqs,magnitudes,channel);
    if(m_waterfallBuffer.size()>=WATERFALL_DEPTH) m_waterfallBuffer.pop_front();
    m_waterfallBuffer.push_back(magnitudes);
    emit waterfallDataReady(m_waterfallBuffer,channel);
    detectSignals(magnitudes,freqs);
}
void SignalProcessor::detectSignals(const QVector<double> &mags, const QVector<double> &freqs) {
    for(int i=1;i<mags.size()-1;i++){
        if(mags[i]>m_threshold && mags[i]>mags[i-1] && mags[i]>mags[i+1]){
            DetectedSignal sig;
            sig.frequency=freqs[i];
            sig.power=mags[i];
            sig.snr=calculateSNR(mags,i);
            sig.bandwidth=calculateBandwidth(mags,i);
            sig.type=classifySignal(sig);
            sig.modulation="FM";
            sig.quality=(sig.snr>20)?"Отличный":(sig.snr>10)?"Хороший":"Средний";
            sig.timestamp=QDateTime::currentSecsSinceEpoch();
            emit signalDetected(sig,0);
        }
    }
}
double SignalProcessor::calculateSNR(const QVector<double> &mags, int peakIdx) {
    double sum=0; int count=0;
    for(int i=0;i<mags.size();i++){ if(i<peakIdx-5 || i>peakIdx+5){ sum+=mags[i]; count++; } }
    double noise=(count>0)?sum/count:0;
    return mags[peakIdx]-noise;
}
double SignalProcessor::calculateBandwidth(const QVector<double> &mags, int peakIdx) {
    double halfPower=mags[peakIdx]-3.0;
    int left=peakIdx, right=peakIdx;
    while(left>0 && mags[left]>halfPower) left--;
    while(right<mags.size()-1 && mags[right]>halfPower) right++;
    return (right-left)*1.0;
}
QString SignalProcessor::classifySignal(const DetectedSignal &info) {
    if(info.bandwidth<1000 && info.snr>15) return "CW";
    if(info.bandwidth<100000 && info.snr>10) return "AM";
    if(info.bandwidth>100000 && info.snr>15) return "FM";
    if(info.bandwidth>1000000) return "WiFi";
    if(info.frequency>2400000000 && info.frequency<2500000000) return "DJI";
    if(info.frequency>400000000 && info.frequency<500000000) return "LORA";
    if(info.bandwidth>200000 && info.bandwidth<1000000) return "FPV Analog";
    return "Unknown";
}
