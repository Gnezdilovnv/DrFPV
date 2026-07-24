#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>
#include "SDRController.h"
#include "SignalProcessor.h"
#include "Visualizer.h"
#include "RecordManager.h"
#include "SettingsManager.h"
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent=nullptr);
    ~MainWindow();
private slots:
    void onConnect();
    void onDisconnect();
    void onStartScan();
    void onStopScan();
    void onToggleRecord();
    void onScreenshot();
    void onFullscreen();
    void onSettings();
    void onAbout();
    void updateUI();
    void onIQData(const QVector<std::complex<float>> &data, int channel);
    void onSpectrum(const QVector<double> &freqs, const QVector<double> &mags, int channel);
    void onSignalDetected(const DetectedSignal &signal, int channel);
    void onWaterfall(const QVector<QVector<double>> &waterfall, int channel);
    void onStatusMessage(const QString &msg);
    void onError(const QString &error);
    void onRecordingStarted(const QString &file);
    void onRecordingStopped(const QString &file);
private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();
    void loadSettings();
    void saveSettings();
    void addLogMessage(const QString &msg);
    void updateSignalList(const DetectedSignal &signal);
    SDRController *m_sdr;
    SignalProcessor *m_processor;
    Visualizer *m_visualizer;
    RecordManager *m_recorder;
    SettingsManager *m_settings;
    QSystemTrayIcon *m_trayIcon;
    QTabWidget *m_tabWidget;
    QTextEdit *m_logWidget;
    QListWidget *m_historyWidget;
    QTimer *m_statusTimer;
    QLabel *m_statusLabel;
    bool m_scanning=false, m_recording=false;
    QVector<DetectedSignal> m_signals;
};
#endif
