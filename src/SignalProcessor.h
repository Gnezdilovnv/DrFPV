#ifndef SIGNALPROCESSOR_H
#define SIGNALPROCESSOR_H
#include <QObject>
#include <QVector>
#include <complex>
#include <QDateTime>
struct DetectedSignal {
    double frequency, power, snr, bandwidth;
    QString type, modulation, quality;
    double timestamp;
};
class SignalProcessor : public QObject {
    Q_OBJECT
public:
    explicit SignalProcessor(QObject *parent=nullptr);
    void setThreshold(double th){ m_threshold=th; }
    void setScanRange(double start,double end){ m_startFreq=start; m_endFreq=end; }
    void processIQData(const QVector<std::complex<float>> &data, double sampleRate, double centerFreq, int channel=0);
signals:
    void spectrumReady(const QVector<double> &freqs, const QVector<double> &mags, int channel);
    void signalDetected(const DetectedSignal &signal, int channel);
    void waterfallDataReady(const QVector<QVector<double>> &waterfall, int channel);
private:
    void detectSignals(const QVector<double> &mags, const QVector<double> &freqs);
    double calculateSNR(const QVector<double> &mags, int peakIdx);
    double calculateBandwidth(const QVector<double> &mags, int peakIdx);
    QString classifySignal(const DetectedSignal &info);
    double m_threshold=10.0;
    double m_startFreq=70e6, m_endFreq=6000e6;
    QVector<QVector<double>> m_waterfallBuffer;
    static const int WATERFALL_DEPTH=200;
};
#endif
