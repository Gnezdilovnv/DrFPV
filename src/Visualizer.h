#ifndef VISUALIZER_H
#define VISUALIZER_H
#include <QWidget>
#include <QVector>
#include <complex>
#include <QImage>
class Visualizer : public QWidget {
    Q_OBJECT
public:
    explicit Visualizer(QWidget *parent=nullptr);
    void setSpectrumData(const QVector<double> &freqs, const QVector<double> &mags);
    void setWaterfallData(const QVector<QVector<double>> &waterfall);
    void setVideoFrame(const QImage &frame);
    void clearVideo();
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    void drawSpectrum(QPainter &painter);
    void drawWaterfall(QPainter &painter);
    void drawVideo(QPainter &painter);
    QVector<double> m_freqs, m_mags;
    QVector<QVector<double>> m_waterfall;
    QImage m_videoFrame;
    int m_currentMode=0;
};
#endif
