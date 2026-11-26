#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QSerialPortInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new QSerialPort(this))
{
    ui->setupUi(this);

    ui->baud_rate_spinBox->setRange(1, 10000000);
    ui->baud_rate_spinBox->setValue(115200);
    ui->log_title->setText("读取数据");
    ui->log_output_browser->document()->setMaximumBlockCount(2000);

    m_rxFlushTimer.setSingleShot(true);
    connect(&m_rxFlushTimer, &QTimer::timeout, this, &MainWindow::flushRxBuffer);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::onSerialError);

    refreshPorts();
}

MainWindow::~MainWindow()
{
    if (m_serial->isOpen()) {
        m_serial->close();
    }
    delete ui;
}

void MainWindow::refreshPorts()
{
    const QString current = ui->port_comboBox->currentText();
    ui->port_comboBox->clear();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        ui->port_comboBox->addItem(info.portName());
    }

    const int index = ui->port_comboBox->findText(current);
    if (index >= 0) {
        ui->port_comboBox->setCurrentIndex(index);
    }

    appendLog(QString("刷新串口完成，发现 %1 个设备").arg(ports.size()));
}

bool MainWindow::ensurePortOpen()
{
    if (m_serial->isOpen()) {
        return true;
    }

    const QString portName = ui->port_comboBox->currentText().trimmed();
    if (portName.isEmpty()) {
        appendLog("请先选择串口");
        return false;
    }

    m_serial->setPortName(portName);
    m_serial->setBaudRate(static_cast<qint32>(ui->baud_rate_spinBox->value()));
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        appendLog(QString("打开串口 %1 失败：%2").arg(portName, m_serial->errorString()));
        return false;
    }

    appendLog(QString("已打开 %1，波特率 %2").arg(portName).arg(ui->baud_rate_spinBox->value()));
    return true;
}

void MainWindow::on_refresh_pushButton_clicked()
{
    refreshPorts();
}

void MainWindow::on_refresh_pushButton_2_clicked()
{
    if (!ensurePortOpen()) {
        return;
    }
    readSerialData();
}

void MainWindow::on_refresh_pushButton_3_clicked()
{
    ui->log_output_browser->clear();
}

void MainWindow::on_input_lineEdit_returnPressed()
{
    if (!ensurePortOpen()) {
        return;
    }

    const QString text = ui->input_lineEdit->text();
    const qint64 written = m_serial->write(text.toUtf8());
    if (written < 0) {
        appendLog(QString("发送失败：%1").arg(m_serial->errorString()));
        return;
    }
    m_serial->flush();
    appendLog(QString("发送：%1").arg(text));
}

static QString decodeSerialData(const QByteArray &data)
{
    const QString utf8 = QString::fromUtf8(data);
    if (!utf8.contains(QChar(0xFFFD))) {
        return utf8;
    }
    return QString::fromLocal8Bit(data);
}

void MainWindow::readSerialData()
{
    m_rxBuffer.append(m_serial->readAll());
    m_rxFlushTimer.start(50);

    int lineEnd = m_rxBuffer.indexOf('\n');
    while (lineEnd >= 0) {
        QByteArray line = m_rxBuffer.left(lineEnd);
        m_rxBuffer.remove(0, lineEnd + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        if (!line.isEmpty()) {
            appendLog("接收：" + decodeSerialData(line));
        }
        lineEnd = m_rxBuffer.indexOf('\n');
    }
}

void MainWindow::flushRxBuffer()
{
    if (m_rxBuffer.isEmpty()) {
        return;
    }
    QByteArray line = m_rxBuffer;
    m_rxBuffer.clear();
    if (line.endsWith('\r')) {
        line.chop(1);
    }
    appendLog("接收：" + decodeSerialData(line));
}

void MainWindow::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        appendLog(QString("串口错误：%1").arg(m_serial->errorString()));
    }
}

void MainWindow::appendLog(const QString &text)
{
    ui->log_output_browser->append(
        QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), text));
}
