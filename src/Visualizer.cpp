#include "Visualizer.h"
#include <QPainter>
#include <QLinearGradient>
Visualizer::Visualizer(QWidget *parent) : QWidget(parent) { setMinimumSize(400,300); }
void Visualizer::setSpectrumData(const QVector<double> &freqs, const QVector<double> &mags) {
    m_freqs=freqs; m_mags=mags; m_currentMode=0; update();
}
void Visualizer::setWaterfallData(const QVector<QVector<double>> &waterfall) {
    m_waterfall=waterfall; m_currentMode=1; update();
}
void Visualizer::setVideoFrame(const QImage &frame) {
    m_videoFrame=frame; m_currentMode=2; update();
}
void Visualizer::clearVideo() { m_videoFrame=QImage(); update(); }
void Visualizer::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(20,20,20));
    if(m_currentMode==0) drawSpectrum(p);
    else if(m_currentMode==1) drawWaterfall(p);
    else if(m_currentMode==2) drawVideo(p);
}
void Visualizer::drawSpectrum(QPainter &p) {
    if(m_mags.isEmpty()){ p.setPen(Qt::white); p.drawText(rect(), Qt::AlignCenter, "Нет данных спектра"); return; }
    QPen pen(QColor(0,255,0)); p.setPen(pen);
    double w=width(), h=height();
    for(int i=0;i<m_mags.size()-1;i++){
        double x1=w*i/m_mags.size(), x2=w*(i+1)/m_mags.size();
        double y1=h-(m_mags[i]+100)/150*h, y2=h-(m_mags[i+1]+100)/150*h;
        p.drawLine(x1,y1,x2,y2);
    }
    p.setPen(Qt::white);
    p.drawText(10,20,"Спектрограмма");
    if(!m_freqs.isEmpty()) p.drawText(10,40,QString("Freq: %.2f МГц").arg(m_freqs[m_freqs.size()/2]/1e6));
}
void Visualizer::drawWaterfall(QPainter &p) {
    if(m_waterfall.isEmpty()){ p.setPen(Qt::white); p.drawText(rect(), Qt::AlignCenter, "Нет данных водопада"); return; }
    double w=width(), h=height(), rowH=h/m_waterfall.size();
    for(int row=0;row<m_waterfall.size();row++){
        const auto &rowData=m_waterfall[row];
        if(rowData.isEmpty()) continue;
        for(int col=0;col<rowData.size()-1;col++){
            double x=w*col/rowData.size(), x2=w*(col+1)/rowData.size(), y=row*rowH;
            double val=qBound(0.0,(rowData[col]+100)/150,1.0);
            QColor color=QColor::fromRgbF(val,val*0.5,1.0-val);
            p.fillRect(QRectF(x,y,x2-x,rowH),color);
        }
    }
    p.setPen(Qt::white); p.drawText(10,20,"Водопад");
}
void Visualizer::drawVideo(QPainter &p) {
    if(m_videoFrame.isNull()){ p.setPen(Qt::white); p.drawText(rect(), Qt::AlignCenter, "Нет видео"); return; }
    QRect target=rect();
    QImage scaled=m_videoFrame.scaled(target.size(), Qt::KeepAspectRatio);
    p.drawImage(target,scaled);
    p.setPen(Qt::white); p.drawText(10,20,"Видео");
}
