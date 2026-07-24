#include "RecordManager.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
RecordManager::RecordManager(QObject *parent) : QObject(parent) {}
void RecordManager::setConfig(const RecordConfig &config){
    m_config=config;
    QDir().mkpath(m_config.videoFolder);
    QDir().mkpath(m_config.iqFolder);
    QDir().mkpath(m_config.spectrogramFolder);
    QDir().mkpath(m_config.reportsFolder);
}
bool RecordManager::startRecording(const QString &filename){
    if(m_recording) return false;
    m_recording=true;
    m_videoBuffer.clear();
    m_currentFile=filename.isEmpty()? m_config.videoFolder+"/rec_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+".mp4": filename;
    emit recordingStarted(m_currentFile);
    emit statusMessage("Запись начата: "+m_currentFile);
    return true;
}
void RecordManager::stopRecording(){
    if(!m_recording) return;
    m_recording=false;
    emit recordingStopped(m_currentFile);
    emit statusMessage("Запись остановлена");
    if(!m_videoBuffer.isEmpty() && m_config.saveVideo)
        emit statusMessage("Сохранено "+QString::number(m_videoBuffer.size())+" кадров");
    m_videoBuffer.clear();
}
void RecordManager::saveIQData(const QVector<std::complex<float>> &data, double frequency){
    if(!m_config.saveIQ || data.isEmpty()) return;
    QString filename=m_config.iqFolder+"/iq_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+".iq";
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly)){
        file.write((const char*)data.data(), data.size()*sizeof(std::complex<float>));
        file.close();
        emit statusMessage("IQ сохранён: "+filename);
    }
}
void RecordManager::saveSpectrogram(const QVector<double> &freqs, const QVector<double> &mags){
    if(!m_config.saveSpectrogram || freqs.isEmpty()) return;
    QString filename=m_config.spectrogramFolder+"/spec_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+".csv";
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly)){
        QTextStream out(&file);
        for(int i=0;i<freqs.size() && i<mags.size();i++) out<<freqs[i]<<","<<mags[i]<<"\n";
        file.close();
        emit statusMessage("Спектрограмма сохранена: "+filename);
    }
}
void RecordManager::saveVideoFrame(const QImage &frame){
    if(!m_recording || !m_config.saveVideo) return;
    m_videoBuffer.append(frame);
    if(m_videoBuffer.size()>1000){ emit statusMessage("Буфер видео переполнен"); m_videoBuffer.clear(); }
}
void RecordManager::saveReport(const QString &text){
    if(!m_config.saveReports) return;
    QString filename=m_config.reportsFolder+"/report_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+".txt";
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly)){
        QTextStream out(&file);
        out<<text;
        file.close();
        emit statusMessage("Отчёт сохранён: "+filename);
    }
}
