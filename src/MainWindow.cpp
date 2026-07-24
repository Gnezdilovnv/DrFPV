#ifdef HAVE_TEXT_TO_SPEECH
#include <QTextToSpeech>
#endif
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QStatusBar>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QShortcut>
#include <QTextEdit>
#include <QListWidget>
#include <QDateTime>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_sdr(new SDRController(this)), m_processor(new SignalProcessor(this)), m_visualizer(new Visualizer(this)), m_recorder(new RecordManager(this)), m_settings(new SettingsManager(this)) {
    #ifdef HAVE_TEXT_TO_SPEECH
    m_speech = new QTextToSpeech(this);
#endif
    setupUI();
    loadSettings();
    connect(m_sdr,&SDRController::statusMessage,this,&MainWindow::onStatusMessage);
    connect(m_sdr,&SDRController::errorOccurred,this,&MainWindow::onError);
    connect(m_sdr,&SDRController::iqDataReady,this,&MainWindow::onIQData);
    connect(m_processor,&SignalProcessor::spectrumReady,this,&MainWindow::onSpectrum);
    connect(m_processor,&SignalProcessor::signalDetected,this,&MainWindow::onSignalDetected);
    connect(m_processor,&SignalProcessor::waterfallDataReady,this,&MainWindow::onWaterfall);
    connect(m_recorder,&RecordManager::recordingStarted,this,&MainWindow::onRecordingStarted);
    connect(m_recorder,&RecordManager::recordingStopped,this,&MainWindow::onRecordingStopped);
    connect(m_recorder,&RecordManager::statusMessage,this,&MainWindow::onStatusMessage);
    m_statusTimer=new QTimer(this);
    connect(m_statusTimer,&QTimer::timeout,this,&MainWindow::updateUI);
    m_statusTimer->start(1000);
    m_trayIcon=new QSystemTrayIcon(QIcon(),this);
    m_trayIcon->setToolTip("DrFPV v8.0");
}
MainWindow::~MainWindow(){ saveSettings(); }
void MainWindow::setupUI(){
    setWindowTitle("DrFPV v8.0");
    resize(1280,800);
    createMenuBar();
    createToolBar();
    createStatusBar();
    createCentralWidget();
    QShortcut *fs=new QShortcut(QKeySequence("F11"),this); connect(fs,&QShortcut::activated,this,&MainWindow::onFullscreen);
    QShortcut *c=new QShortcut(QKeySequence("Ctrl+C"),this); connect(c,&QShortcut::activated,this,&MainWindow::onConnect);
    QShortcut *s=new QShortcut(QKeySequence("Ctrl+S"),this); connect(s,&QShortcut::activated,this,&MainWindow::onStartScan);
    QShortcut *r=new QShortcut(QKeySequence("Ctrl+R"),this); connect(r,&QShortcut::activated,this,&MainWindow::onToggleRecord);
}
void MainWindow::createMenuBar(){
    auto mb=new QMenuBar(this);
    auto file=mb->addMenu("Файл");
    file->addAction("Экспорт отчёта",this,[this](){ m_recorder->saveReport("Отчёт DrFPV"); });
    file->addAction("Выход",this,&QWidget::close);
    auto ctrl=mb->addMenu("Управление");
    ctrl->addAction("Подключить",this,&MainWindow::onConnect);
    ctrl->addAction("Отключить",this,&MainWindow::onDisconnect);
    ctrl->addAction("Старт сканирования",this,&MainWindow::onStartScan);
    ctrl->addAction("Стоп",this,&MainWindow::onStopScan);
    ctrl->addAction("Запись",this,&MainWindow::onToggleRecord);
    ctrl->addAction("Снимок",this,&MainWindow::onScreenshot);
    auto view=mb->addMenu("Вид");
    view->addAction("Полный экран",this,&MainWindow::onFullscreen);
    auto settings=mb->addMenu("Настройки");
    settings->addAction("Настройки",this,&MainWindow::onSettings);
    auto help=mb->addMenu("Помощь");
    help->addAction("О программе",this,&MainWindow::onAbout);
    setMenuBar(mb);
}
void MainWindow::createToolBar(){
    auto tb=addToolBar("Главная");
    tb->addAction("Подключить",this,&MainWindow::onConnect);
    tb->addAction("Старт",this,&MainWindow::onStartScan);
    tb->addAction("Стоп",this,&MainWindow::onStopScan);
    tb->addAction("Запись",this,&MainWindow::onToggleRecord);
    tb->addAction("Снимок",this,&MainWindow::onScreenshot);
}
void MainWindow::createStatusBar(){
    m_statusLabel=new QLabel("Готов");
    statusBar()->addWidget(m_statusLabel);
}
void MainWindow::createCentralWidget(){
    auto main=new QHBoxLayout;
    auto left=new QVBoxLayout;
    QPushButton *connectBtn=new QPushButton("Подключить"); connect(connectBtn,&QPushButton::clicked,this,&MainWindow::onConnect);
    QPushButton *startBtn=new QPushButton("Старт"); connect(startBtn,&QPushButton::clicked,this,&MainWindow::onStartScan);
    QPushButton *stopBtn=new QPushButton("Стоп"); connect(stopBtn,&QPushButton::clicked,this,&MainWindow::onStopScan);
    QPushButton *recordBtn=new QPushButton("Запись"); connect(recordBtn,&QPushButton::clicked,this,&MainWindow::onToggleRecord);
    left->addWidget(connectBtn); left->addWidget(startBtn); left->addWidget(stopBtn); left->addWidget(recordBtn);
    left->addWidget(new QLabel("Список сигналов:"));
    m_historyWidget=new QListWidget; left->addWidget(m_historyWidget);
    left->addStretch();
    m_tabWidget=new QTabWidget;
    auto videoW=new QWidget; auto vl=new QVBoxLayout(videoW); vl->addWidget(m_visualizer);
    auto specW=new QWidget; auto sl=new QVBoxLayout(specW); sl->addWidget(new QLabel("Спектрограмма"));
    auto wfW=new QWidget; auto wfl=new QVBoxLayout(wfW); wfl->addWidget(new QLabel("Водопад"));
    auto logW=new QWidget; auto ll=new QVBoxLayout(logW); ll->addWidget(new QLabel("Лог"));
    m_logWidget=new QTextEdit; m_logWidget->setReadOnly(true); ll->addWidget(m_logWidget);
    m_tabWidget->addTab(videoW,"Видео");
    m_tabWidget->addTab(specW,"Спектр");
    m_tabWidget->addTab(wfW,"Водопад");
    m_tabWidget->addTab(logW,"Лог");
    main->addLayout(left,1); main->addWidget(m_tabWidget,3);
    auto cw=new QWidget; cw->setLayout(main); setCentralWidget(cw);
}
void MainWindow::onConnect(){
    if(m_sdr->isConnected()){ onDisconnect(); return; }
    QString ip=m_settings->value("SDR/ip","192.168.2.1").toString();
    if(m_sdr->connectToDevice(ip)){
        SDRConfig cfg;
        cfg.ip=ip;
        cfg.rxFrequency=m_settings->value("SDR/rx_frequency",2.4e9).toDouble();
        cfg.sampleRate=m_settings->value("SDR/sample_rate",5e6).toDouble();
        cfg.rxBandwidth=m_settings->value("SDR/rx_bandwidth",18e6).toDouble();
        cfg.rxGain1=m_settings->value("SDR/rx_gain1",30.0).toDouble();
        cfg.rxGain2=m_settings->value("SDR/rx_gain2",30.0).toDouble();
        cfg.gainMode=m_settings->value("SDR/gain_mode","manual").toString();
        cfg.enable2R2T=m_settings->value("SDR/enable_2r2t",true).toBool();
        cfg.agcEnabled=m_settings->value("SDR/agc_enabled",false).toBool();
        cfg.dcOffsetTracking=m_settings->value("SDR/dc_offset_tracking",true).toBool();
        cfg.quadratureTracking=m_settings->value("SDR/quadrature_tracking",true).toBool();
        cfg.firFiltersEnabled=m_settings->value("SDR/fir_filters_enabled",true).toBool();
        m_sdr->configure(cfg);
        m_sdr->startStream();
        addLogMessage("Подключено к PlutoSDR ("+ip+")");
    }
}
void MainWindow::onDisconnect(){ m_sdr->stopStream(); m_sdr->disconnectDevice(); addLogMessage("Отключено"); }
void MainWindow::onStartScan(){ m_scanning=true; addLogMessage("Сканирование запущено"); m_statusLabel->setText("Сканирование..."); }
void MainWindow::onStopScan(){ m_scanning=false; addLogMessage("Сканирование остановлено"); m_statusLabel->setText("Остановлено"); }
void MainWindow::onToggleRecord(){
    m_recording=!m_recording;
    if(m_recording){ m_recorder->startRecording(); addLogMessage("Запись начата"); }
    else { m_recorder->stopRecording(); addLogMessage("Запись остановлена"); }
}
void MainWindow::onScreenshot(){ m_recorder->saveSpectrogram(QVector<double>(),QVector<double>()); addLogMessage("Снимок сохранён"); }
void MainWindow::onFullscreen(){ if(isFullScreen()) showNormal(); else showFullScreen(); }
void MainWindow::onSettings(){
    QDialog d(this); d.setWindowTitle("Настройки");
    auto layout=new QVBoxLayout(&d);
    auto tabs=new QTabWidget;
    auto sdr=new QWidget; auto f=new QFormLayout(sdr);
    QLineEdit *ip=new QLineEdit(m_settings->value("SDR/ip","192.168.2.1").toString());
    f->addRow("IP:",ip);
    QDoubleSpinBox *freq=new QDoubleSpinBox; freq->setRange(70e6,6000e6); freq->setValue(m_settings->value("SDR/rx_frequency",2.4e9).toDouble()); freq->setSuffix(" Гц");
    f->addRow("Частота RX:",freq);
    QDoubleSpinBox *g1=new QDoubleSpinBox; g1->setRange(0,73); g1->setValue(m_settings->value("SDR/rx_gain1",30.0).toDouble());
    f->addRow("Усиление RX1:",g1);
    QDoubleSpinBox *g2=new QDoubleSpinBox; g2->setRange(0,73); g2->setValue(m_settings->value("SDR/rx_gain2",30.0).toDouble());
    f->addRow("Усиление RX2:",g2);
    QCheckBox *agc=new QCheckBox; agc->setChecked(m_settings->value("SDR/agc_enabled",false).toBool());
    f->addRow("AGC:",agc);
    QCheckBox *dc=new QCheckBox; dc->setChecked(m_settings->value("SDR/dc_offset_tracking",true).toBool());
    f->addRow("DC Offset:",dc);
    QCheckBox *quad=new QCheckBox; quad->setChecked(m_settings->value("SDR/quadrature_tracking",true).toBool());
    f->addRow("Quadrature:",quad);
    QCheckBox *fir=new QCheckBox; fir->setChecked(m_settings->value("SDR/fir_filters_enabled",true).toBool());
    f->addRow("FIR:",fir);
    QCheckBox *r2t=new QCheckBox; r2t->setChecked(m_settings->value("SDR/enable_2r2t",true).toBool());
    f->addRow("2R2T:",r2t);
    tabs->addTab(sdr,"SDR");
    auto scan=new QWidget; auto sf=new QFormLayout(scan);
    QDoubleSpinBox *start=new QDoubleSpinBox; start->setRange(70e6,6000e6); start->setValue(m_settings->value("Scan/start_freq",70e6).toDouble());
    sf->addRow("Нач. частота:",start);
    QDoubleSpinBox *end=new QDoubleSpinBox; end->setRange(70e6,6000e6); end->setValue(m_settings->value("Scan/end_freq",6000e6).toDouble());
    sf->addRow("Кон. частота:",end);
    QDoubleSpinBox *thr=new QDoubleSpinBox; thr->setRange(0,100); thr->setValue(m_settings->value("Scan/threshold",10.0).toDouble());
    sf->addRow("Порог (дБ):",thr);
    tabs->addTab(scan,"Сканирование");
    layout->addWidget(tabs);
    auto btns=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    layout->addWidget(btns);
    connect(btns,&QDialogButtonBox::accepted,&d,&QDialog::accept);
    connect(btns,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    if(d.exec()==QDialog::Accepted){
        m_settings->setValue("SDR/ip",ip->text());
        m_settings->setValue("SDR/rx_frequency",freq->value());
        m_settings->setValue("SDR/rx_gain1",g1->value());
        m_settings->setValue("SDR/rx_gain2",g2->value());
        m_settings->setValue("SDR/agc_enabled",agc->isChecked());
        m_settings->setValue("SDR/dc_offset_tracking",dc->isChecked());
        m_settings->setValue("SDR/quadrature_tracking",quad->isChecked());
        m_settings->setValue("SDR/fir_filters_enabled",fir->isChecked());
        m_settings->setValue("SDR/enable_2r2t",r2t->isChecked());
        m_settings->setValue("Scan/start_freq",start->value());
        m_settings->setValue("Scan/end_freq",end->value());
        m_settings->setValue("Scan/threshold",thr->value());
        m_settings->sync();
        addLogMessage("Настройки сохранены");
    }
}
void MainWindow::onAbout(){
    QMessageBox::about(this,"О DrFPV",
        "<h2>DrFPV v8.0</h2><p>Профессиональный анализатор сигналов дронов</p>"
        "<p>Поддерживаемые типы: DJI, LORA, FPV, WiFi, CW, AM, FM</p>"
        "<p>Работает с PlutoSDR (70 МГц - 6 ГГц)</p>"
        "<p><b>Горячие клавиши:</b></p><ul><li>Ctrl+C - Подключить</li>"
        "<li>Ctrl+S - Старт сканирования</li><li>Ctrl+R - Запись</li>"
        "<li>F11 - Полный экран</li></ul><p>Лицензия MIT</p><p>© 2026 DrFPV Team</p>");
}
void MainWindow::updateUI(){
    if(m_sdr->isConnected()){
        double rssi1=m_sdr->getRSSI(0), rssi2=m_sdr->getRSSI(1);
        double temp=m_sdr->getTemperature(), volt=m_sdr->getVoltage();
        m_statusLabel->setText(QString("RSSI1: %1 dBm | RSSI2: %2 dBm | Temp: %3°C | V: %4V")
            .arg(rssi1,0,'f',1).arg(rssi2,0,'f',1).arg(temp,0,'f',1).arg(volt,0,'f',2));
    }
}
void MainWindow::onIQData(const QVector<std::complex<float>> &data, int channel){
    if(m_scanning && !data.isEmpty()){
        double freq=m_settings->value("SDR/rx_frequency",2.4e9).toDouble();
        double rate=m_settings->value("SDR/sample_rate",5e6).toDouble();
        m_processor->processIQData(data,rate,freq,channel);
        if(m_recording) m_recorder->saveIQData(data,freq);
    }
}
void MainWindow::onSpectrum(const QVector<double> &freqs, const QVector<double> &mags, int channel){
    m_visualizer->setSpectrumData(freqs,mags);
    if(m_recording) m_recorder->saveSpectrogram(freqs,mags);
}
void MainWindow::onSignalDetected(const DetectedSignal &signal, int channel){
    addLogMessage(QString("Обнаружен %1 на %2 МГц (канал %3)").arg(signal.type).arg(signal.frequency/1e6,0,'f',2).arg(channel));
    updateSignalList(signal);
    if(m_recording) m_recorder->saveReport(QString("Обнаружен %1 на %2 МГц, SNR=%3 дБ, ширина=%4 Гц")
            .arg(signal.type).arg(signal.frequency/1e6,0,'f',2).arg(signal.snr,0,'f',1).arg(signal.bandwidth,0,'f',0));
}
void MainWindow::onWaterfall(const QVector<QVector<double>> &waterfall, int channel){
    m_visualizer->setWaterfallData(waterfall);
}
void MainWindow::onStatusMessage(const QString &msg){ addLogMessage(msg); }
void MainWindow::onError(const QString &err){ addLogMessage("ОШИБКА: "+err); QMessageBox::critical(this,"Ошибка",err); }
void MainWindow::onRecordingStarted(const QString &file){ addLogMessage("Запись начата: "+file); }
void MainWindow::onRecordingStopped(const QString &file){ addLogMessage("Запись остановлена: "+file); }
void MainWindow::addLogMessage(const QString &msg){ m_logWidget->append("["+QDateTime::currentDateTime().toString("hh:mm:ss")+"] "+msg); }
void MainWindow::updateSignalList(const DetectedSignal &signal){
    QString item=QString("%1 МГц | %2 | SNR: %3 дБ").arg(signal.frequency/1e6,0,'f',2).arg(signal.type).arg(signal.snr,0,'f',1);
    m_historyWidget->addItem(item);
    if(m_historyWidget->count()>100) delete m_historyWidget->takeItem(0);
}
void MainWindow::loadSettings(){
    RecordConfig rc;
    rc.saveVideo=m_settings->value("Recording/save_video",true).toBool();
    rc.saveIQ=m_settings->value("Recording/save_iq",false).toBool();
    rc.saveSpectrogram=m_settings->value("Recording/save_spectrogram",true).toBool();
    rc.saveReports=m_settings->value("Recording/save_reports",true).toBool();
    rc.videoFolder=m_settings->value("Paths/video","./video").toString();
    rc.iqFolder=m_settings->value("Paths/iq","./iq").toString();
    rc.spectrogramFolder=m_settings->value("Paths/spectrograms","./spectrograms").toString();
    rc.reportsFolder=m_settings->value("Paths/reports","./reports").toString();
    m_recorder->setConfig(rc);
    m_processor->setThreshold(m_settings->value("Scan/threshold",10.0).toDouble());
    m_processor->setScanRange(m_settings->value("Scan/start_freq",70e6).toDouble(),
                              m_settings->value("Scan/end_freq",6000e6).toDouble());
}
void MainWindow::saveSettings(){ m_settings->sync(); }
#ifdef HAVE_TEXT_TO_SPEECH
void MainWindow::voiceAlert(const QString &msg){
    if(m_speech && m_settings->value("Alerts/voice_enabled",true).toBool()){
        m_speech->say(msg);
    }
#endif
}
#endif
