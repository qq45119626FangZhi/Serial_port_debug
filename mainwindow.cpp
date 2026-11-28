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

    ui->log_output_browser->document()->setMaximumBlockCount(2000);

    m_rxFlushTimer.setSingleShot(true);
    connect(&m_rxFlushTimer, &QTimer::timeout, this, &MainWindow::flushRxBuffer);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::onSerialError);

    refreshPorts();
    updateStatusLabel();
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

    const int baudRate = ui->baud_comboBox->currentText().toInt();

    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        appendLog(QString("打开串口 %1 失败：%2").arg(portName, m_serial->errorString()));
        updateStatusLabel();
        return false;
    }

    m_rxBuffer.clear();
    appendLog(QString("已打开 %1，波特率 %2").arg(portName).arg(baudRate));
    updateStatusLabel();
    return true;
}

void MainWindow::closePort()
{
    if (!m_serial->isOpen()) {
        return;
    }

    const QString portName = m_serial->portName();
    m_serial->close();
    m_rxBuffer.clear();
    m_rxFlushTimer.stop();
    appendLog(QString("已关闭 %1").arg(portName));
    updateStatusLabel();
}

void MainWindow::updateStatusLabel()
{
    if (m_serial->isOpen()) {
        ui->status_label->setText(
            QString("已连接: %1 @ %2").arg(m_serial->portName()).arg(m_serial->baudRate()));
    } else {
        ui->status_label->setText("未连接");
    }
}

static QByteArray parseHexInput(const QString &text)
{
    QByteArray data;
    const QString cleaned = text.simplified();
    const QString hex = QString(cleaned).remove(' ');
    if (hex.isEmpty()) {
        return data;
    }
    if (hex.length() % 2 != 0) {
        return QByteArray();
    }
    for (int i = 0; i < hex.length(); i += 2) {
        bool ok = false;
        const int value = hex.mid(i, 2).toInt(&ok, 16);
        if (!ok) {
            return QByteArray();
        }
        data.append(static_cast<char>(value));
    }
    return data;
}

void MainWindow::sendData()
{
    if (!ensurePortOpen()) {
        return;
    }

    const QString text = ui->input_lineEdit->text();
    QByteArray payload;

    if (ui->hex_checkBox->isChecked()) {
        payload = parseHexInput(text);
        if (payload.isEmpty()) {
            appendLog("HEX 格式错误，请输入偶数个十六进制字符（如 55 AA 01）");
            return;
        }
    } else {
        payload = text.toUtf8();
    }

    if (payload.isEmpty()) {
        appendLog("发送内容为空");
        return;
    }

    const qint64 written = m_serial->write(payload);
    if (written < 0) {
        appendLog(QString("发送失败：%1").arg(m_serial->errorString()));
        return;
    }
    m_serial->flush();

    if (ui->hex_checkBox->isChecked()) {
        appendLog(QString("发送(HEX)：%1").arg(QString::fromLatin1(payload.toHex(' '))));
    } else {
        appendLog(QString("发送：%1").arg(text));
    }
}

void MainWindow::on_refresh_pushButton_clicked()
{
    refreshPorts();
}

void MainWindow::on_close_pushButton_clicked()
{
    closePort();
}

void MainWindow::on_send_pushButton_clicked()
{
    sendData();
}

void MainWindow::on_clear_pushButton_clicked()
{
    ui->log_output_browser->clear();
}

void MainWindow::on_baud_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_serial->isOpen()) {
        const int baudRate = ui->baud_comboBox->currentText().toInt();
        m_serial->setBaudRate(baudRate);
        appendLog(QString("波特率已调整为 %1").arg(baudRate));
        updateStatusLabel();
    }
}

void MainWindow::on_input_lineEdit_returnPressed()
{
    sendData();
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
        updateStatusLabel();
    }
}

void MainWindow::appendLog(const QString &text)
{
    ui->log_output_browser->append(
        QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), text));
}
