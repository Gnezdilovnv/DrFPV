#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H
#include <QObject>
#include <QString>
#include <QVector>
#include <complex>
#include <QImage>
#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

struct RecordConfig {
    bool saveVideo=true, saveIQ=false, saveSpectrogram=true, saveReports=true;
    QString videoFolder="./video", iqFolder="./iq", spectrogramFolder="./spectrograms", reportsFolder="./reports";
};

class RecordManager : public QObject {
    Q_OBJECT
public:
    explicit RecordManager(QObject *parent=nullptr);
    ~RecordManager(){ stopRecording(); }

    void setConfig(const RecordConfig &config);
    bool startRecording(const QString &filename="");
    void stopRecording();
    bool isRecording() const { return m_recording; }

    void saveIQData(const QVector<std::complex<float>> &data, double frequency);
    void saveSpectrogram(const QVector<double> &freqs, const QVector<double> &mags);
    void saveVideoFrame(const QImage &frame);
    void saveReport(const QString &text);

signals:
    void recordingStarted(const QString &file);
    void recordingStopped(const QString &file);
    void statusMessage(const QString &msg);

private:
    RecordConfig m_config;
    bool m_recording=false;
    QString m_currentFile;
    QVector<QImage> m_videoBuffer;
#ifdef USE_OPENCV
    cv::VideoWriter *m_writer=nullptr;
#endif
};
#endif
