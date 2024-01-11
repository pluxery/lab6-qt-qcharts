#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QJsonArray>

#include <QtCharts>
#include <QtCharts/QChart>

ServerAPI::ServerAPI(QObject* parent) : QObject(parent) { }

void ServerAPI::GetTemperatureNow() {
    QNetworkRequest req(QUrl("http://localhost:8000/current-temperature"));
    QNetworkAccessManager* networkManager = new QNetworkAccessManager();
    QNetworkReply* networkReply = networkManager->get(req);

    QObject::connect(networkReply, &QNetworkReply::finished, [networkReply, this]() {
        QString ReplyText = networkReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(ReplyText.toUtf8());
        QJsonObject obj = doc.object();
        QJsonValue val = obj.value("data");
        networkReply->deleteLater();
        double temperature = val.toObject().value("temperature").toDouble();
        emit currentTemperatureSignal(temperature);
    });
}

void ServerAPI::GetTemperatureByInterval(QDateTime begin, QDateTime end) {
    QString beginStartDateTimeString = begin.toString(QString("yyyy-MM-dd hh:mm:ss"));
    QString endDateTimeString = end.toString(QString("yyyy-MM-dd hh:mm:ss"));
    QUrl url = QUrl("http://localhost:8000/temperature-by-range");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray body;
    body.append("start=" + beginStartDateTimeString.toStdString());
    body.append("end=" + endDateTimeString.toStdString());

    QNetworkAccessManager* networkManager = new QNetworkAccessManager();
    QNetworkReply* reply = networkManager->post(req, body);
    QObject::connect(reply, &QNetworkReply::finished, [reply, this]() {
        QString replyStr = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(replyStr.toUtf8());
        QJsonObject obj = doc.object();
        QJsonValue val = obj.value("result");
        QJsonArray rows = val.toArray();

        QLineSeries *series = new QLineSeries();

        for (auto row: rows) {
            double y = row.toObject().value("temperature").toDouble();

            QString xDateTimeString = row.toObject().value("created_at").toString();
            QDateTime xDateTime = QDateTime::fromString(xDateTimeString, "yyyy-MM-dd HH:mm:ss");
            double x = xDateTime.toMSecsSinceEpoch();
            series->append(x, y);
        }

        reply->deleteLater();
        emit temperatureRangeSignal(series);
    });
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer (new QTimer(this))
    , api(new ServerAPI(this))
    , chartView(new QChartView(this)) {
    ui->setupUi(this);

    ui->startDateTimeRange->setDisplayFormat(QString("dd.MM.yyyy hh:mm:ss"));
    ui->startDateTimeRange->setDateTime(QDateTime::currentDateTime().addMonths(-1));

    ui->endDateTimeRange->setDisplayFormat(QString("dd.MM.yyyy hh:mm:ss"));
    ui->endDateTimeRange->setDateTime(QDateTime::currentDateTime());

    api->GetTemperatureByInterval(QDateTime::currentDateTime().addMonths(-1), QDateTime::currentDateTime());

    ui->gridLayout->addWidget(chartView);

    api->GetTemperatureNow();
    timer->setInterval(5000);
    timer->start();

    connect(ui->startDateTimeRange, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(updatePlotStartChange(QDateTime)));
    connect(ui->endDateTimeRange, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(updatePlotEndChange(QDateTime)));
    connect(api,SIGNAL(currentTemperatureSignal(double)), this, SLOT(setCurrentTemperature(double)));
    connect(timer,SIGNAL(timeout()), api,SLOT(getCurrentTemperature()));
    connect(api, SIGNAL(temperatureRangeSignal(QLineSeries*)), this, SLOT(plotChartChange(QLineSeries*)));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::updatePlotStartChange(QDateTime start) {
    api->GetTemperatureByInterval(start, ui->endDateTimeRange->dateTime());
}

void MainWindow::updatePlotEndChange(QDateTime finish) {
    api->GetTemperatureByInterval(ui->startDateTimeRange->dateTime(), finish);
}

void MainWindow::setCurrentTemperature(double temperatureValue) {
    ui->currentTemp->display(temperatureValue);
}

void MainWindow::plotChartChange(QLineSeries *series) {

    QChart *chart = new QChart();
    chart->addSeries(series);

    chart->createDefaultAxes();

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("time");
    axisX->setGridLineVisible(true);
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("temperature");
    axisY->setGridLineVisible(true);
    chart->addAxis(axisY, Qt::AlignLeft);

    chartView->setChart(chart);
    chart->show();
}

