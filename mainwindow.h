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

class Temperature : public QObject {
    Q_OBJECT

public:
    Temperature(QObject* parent);
    virtual ~Temperature(){};
    void getTemperatureRange(QDateTime, QDateTime);
public slots:
    void getCurrentTemperature();
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
    Temperature*    fromServer;
    QChartView* chartView;
};
#endif // MAINWINDOW_H
