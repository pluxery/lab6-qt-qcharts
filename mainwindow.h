#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QJsonArray>
#include <QMainWindow>

#include <QtCharts>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ServerAPI : public QObject {
    Q_OBJECT

public:
    ServerAPI(QObject* parent);
    virtual ~ServerAPI(){};
    void GetTemperatureByInterval(QDateTime, QDateTime);
public slots:
    void GetTemperatureNow();
signals:
    void currentTemperatureSignal(double);
    void temperatureRangeSignal(QLineSeries*);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void setCurrentTemperature(double);
    void updatePlotStartChange(QDateTime);
    void updatePlotEndChange(QDateTime);
    void plotChartChange(QLineSeries*);

private:
    Ui::MainWindow *ui;
    QTimer*         timer;
    ServerAPI*    api;
    QChartView* chartView;
};
#endif // MAINWINDOW_H
