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

Temperature::Temperature(QObject* parent) : QObject(parent) { }

void Temperature::getCurrentTemperature() {

    QTextStream(stdout) << "Get now..." << Qt::endl;

    QNetworkRequest request(QUrl("http://localhost:8000/api/now"));
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QNetworkReply* reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply, this]() {
        QString ReplyText = reply->readAll();
        QJsonDocument jsonDocument = QJsonDocument::fromJson(ReplyText.toUtf8());
        QJsonObject jsonObject = jsonDocument.object();
        QJsonValue jsonValue = jsonObject.value("data");
        reply->deleteLater();
        double temperature = jsonValue.toObject().value("temperature").toDouble();
        QTextStream(stdout) << "Temperature now: " << temperature << Qt::endl;
        emit currentTemperatureSignal(temperature);
    });
}

void Temperature::getTemperatureRange(QDateTime start, QDateTime finish) {
    QTextStream(stdout) << "get interval..." << Qt::endl;

    QString startStr = start.toString(QString("yyyy-MM-dd hh:mm:ss"));
    QString finishStr = finish.toString(QString("yyyy-MM-dd hh:mm:ss"));

    QTextStream(stdout) << "got date inputs: " << startStr << ", " << finishStr << Qt::endl;

    QUrl url = QUrl("http://localhost:8000/api/interval");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray data;
    // data.append("start=2024-01-01");
    // data.append("end=2024-01-05");
    data.append("start=" + startStr.toStdString());
    data.append("end=" + finishStr.toStdString());

    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QNetworkReply* reply = manager->post(request, data);
    QObject::connect(reply, &QNetworkReply::finished, [reply, this]() {
        QString replyStr = reply->readAll();
        QJsonDocument jsonDocument = QJsonDocument::fromJson(replyStr.toUtf8());
        QJsonObject jsonObject = jsonDocument.object();
        QJsonValue jsonValue = jsonObject.value("data");
        QJsonArray points = jsonValue.toArray();

        QLineSeries *series = new QLineSeries();

        for (auto p: points) {
            double yPoint = p.toObject().value("temperature").toDouble();

            QString xDateStr = p.toObject().value("created_at").toString();
            QDateTime xDate = QDateTime::fromString(xDateStr, "yyyy-MM-dd HH:mm:ss");
            double xPoint = xDate.toMSecsSinceEpoch();

            series->append(xPoint, yPoint);

           // QTextStream(stdout) << "got point (x, y): " << xPoint << ", " << yPoint << Qt::endl;
        }

        reply->deleteLater();
        emit temperatureRangeSignal(series);
    });
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer (new QTimer(this))
    , fromServer(new Temperature(this))
    , chartView(new QChartView(this)) {
    ui->setupUi(this);

    ui->startDateTimeRange->setDisplayFormat(QString("dd.MM.yyyy hh:mm:ss"));
    ui->startDateTimeRange->setDateTime(QDateTime::currentDateTime().addMonths(-1));

    ui->endDateTimeRange->setDisplayFormat(QString("dd.MM.yyyy hh:mm:ss"));
    ui->endDateTimeRange->setDateTime(QDateTime::currentDateTime());

    fromServer->getTemperatureRange(QDateTime::currentDateTime().addMonths(-1), QDateTime::currentDateTime());

    ui->gridLayout->addWidget(chartView);

    fromServer->getCurrentTemperature();
    timer->setInterval(5000);
    timer->start();

    connect(ui->startDateTimeRange, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(updatePlotStartChange(QDateTime)));
    connect(ui->endDateTimeRange, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(updatePlotEndChange(QDateTime)));
    connect(fromServer,SIGNAL(currentTemperatureSignal(double)), this, SLOT(setCurrentTemperature(double)));
    connect(timer,SIGNAL(timeout()), fromServer,SLOT(getCurrentTemperature()));
    connect(fromServer, SIGNAL(temperatureRangeSignal(QLineSeries*)), this, SLOT(plotChartChange(QLineSeries*)));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::updatePlotStartChange(QDateTime start) {
    fromServer->getTemperatureRange(start, ui->endDateTimeRange->dateTime());
}

void MainWindow::updatePlotEndChange(QDateTime finish) {
    fromServer->getTemperatureRange(ui->startDateTimeRange->dateTime(), finish);
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

