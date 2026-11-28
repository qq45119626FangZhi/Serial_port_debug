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
    const QString currentName = ui->port_comboBox->currentData().toString();
    ui->port_comboBox->clear();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        const QString label = info.description().isEmpty()
            ? info.portName()
            : QString("%1 - %2").arg(info.portName(), info.description());
        ui->port_comboBox->addItem(label, info.portName());
    }

    const int index = ui->port_comboBox->findData(currentName);
    if (index >= 0) {
        ui->port_comboBox->setCurrentIndex(index);
    }

    appendLog(QString("刷新串口完成，发现 %1 个设备").arg(ports.size()));
}

void MainWindow::applySerialParams()
{
    const int baudRate = ui->baud_comboBox->currentText().toInt();
    const QString dataBits = ui->data_bits_comboBox->currentText();
    const QString parity = ui->parity_comboBox->currentText();
    const QString stopBits = ui->stop_bits_comboBox->currentText();

    m_serial->setBaudRate(baudRate);

    if (dataBits == "5") {
        m_serial->setDataBits(QSerialPort::Data5);
    } else if (dataBits == "6") {
        m_serial->setDataBits(QSerialPort::Data6);
    } else if (dataBits == "7") {
        m_serial->setDataBits(QSerialPort::Data7);
    } else {
        m_serial->setDataBits(QSerialPort::Data8);
    }

    if (parity == "Even") {
        m_serial->setParity(QSerialPort::EvenParity);
    } else if (parity == "Odd") {
        m_serial->setParity(QSerialPort::OddParity);
    } else if (parity == "Space") {
        m_serial->setParity(QSerialPort::SpaceParity);
    } else if (parity == "Mark") {
        m_serial->setParity(QSerialPort::MarkParity);
    } else {
        m_serial->setParity(QSerialPort::NoParity);
    }

    if (stopBits == "1.5") {
        m_serial->setStopBits(QSerialPort::OneAndHalfStop);
    } else if (stopBits == "2") {
        m_serial->setStopBits(QSerialPort::TwoStop);
    } else {
        m_serial->setStopBits(QSerialPort::OneStop);
    }

    m_serial->setFlowControl(QSerialPort::NoFlowControl);
}

bool MainWindow::ensurePortOpen()
{
    if (m_serial->isOpen()) {
        return true;
    }

    const QString portName = ui->port_comboBox->currentData().toString();
    if (portName.isEmpty()) {
        appendLog("请先选择串口");
        return false;
    }

    m_serial->setPortName(portName);
    applySerialParams();

    if (!m_serial->open(QIODevice::ReadWrite)) {
        appendLog(QString("打开串口 %1 失败：%2").arg(portName, m_serial->errorString()));
        updateStatusLabel();
        return false;
    }

    m_rxBuffer.clear();
    appendLog(QString("已打开 %1，波特率 %2，数据位 %3，校验位 %4，停止位 %5")
                  .arg(portName)
                  .arg(ui->baud_comboBox->currentText())
                  .arg(ui->data_bits_comboBox->currentText())
                  .arg(ui->parity_comboBox->currentText())
                  .arg(ui->stop_bits_comboBox->currentText()));
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
        ui->open_pushButton->setText("关闭串口");
    } else {
        ui->status_label->setText("未连接");
        ui->open_pushButton->setText("打开串口");
    }
}

void MainWindow::sendData()
{
    if (!ensurePortOpen()) {
        return;
    }

    const QByteArray payload = ui->input_lineEdit->text().toUtf8();
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
    appendDataLog(false, payload);
}

void MainWindow::on_refresh_pushButton_clicked()
{
    refreshPorts();
}

void MainWindow::on_open_pushButton_clicked()
{
    if (m_serial->isOpen()) {
        closePort();
    } else {
        ensurePortOpen();
    }
}

void MainWindow::on_send_pushButton_clicked()
{
    sendData();
}

void MainWindow::on_clear_pushButton_clicked()
{
    m_logEntries.clear();
    ui->log_output_browser->clear();
}

void MainWindow::on_hex_checkBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    renderLog();
}

void MainWindow::on_rx_checkBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    renderLog();
}

void MainWindow::on_tx_checkBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    renderLog();
}

void MainWindow::on_baud_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_serial->isOpen()) {
        applySerialParams();
        appendLog(QString("波特率已调整为 %1").arg(ui->baud_comboBox->currentText()));
        updateStatusLabel();
    }
}

void MainWindow::on_data_bits_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_serial->isOpen()) {
        applySerialParams();
        appendLog(QString("数据位已调整为 %1").arg(ui->data_bits_comboBox->currentText()));
    }
}

void MainWindow::on_parity_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_serial->isOpen()) {
        applySerialParams();
        appendLog(QString("校验位已调整为 %1").arg(ui->parity_comboBox->currentText()));
    }
}

void MainWindow::on_stop_bits_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_serial->isOpen()) {
        applySerialParams();
        appendLog(QString("停止位已调整为 %1").arg(ui->stop_bits_comboBox->currentText()));
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

static QString formatLogData(const QByteArray &data, bool hexMode)
{
    if (hexMode) {
        return QString::fromLatin1(data.toHex(' ')).toUpper();
    }
    return decodeSerialData(data);
}

bool MainWindow::isEntryVisible(const LogEntry &entry) const
{
    if (entry.type == LogEntry::Rx) {
        return ui->rx_checkBox->isChecked();
    }
    if (entry.type == LogEntry::Tx) {
        return ui->tx_checkBox->isChecked();
    }
    return true;
}

void MainWindow::appendEntryToBrowser(const LogEntry &entry)
{
    QString content;
    if (entry.type == LogEntry::System) {
        content = entry.text;
    } else {
        content = QString("%1：%2").arg(
            entry.type == LogEntry::Rx ? "接收" : "发送",
            formatLogData(entry.data, ui->hex_checkBox->isChecked()));
    }
    ui->log_output_browser->append(QString("[%1] %2").arg(entry.time, content));
}

void MainWindow::appendEntry(const LogEntry &entry)
{
    m_logEntries.append(entry);
    if (m_logEntries.size() > 2000) {
        m_logEntries.removeFirst();
    }
    if (isEntryVisible(entry)) {
        appendEntryToBrowser(entry);
    }
}

void MainWindow::appendLog(const QString &text)
{
    LogEntry entry;
    entry.type = LogEntry::System;
    entry.time = QDateTime::currentDateTime().toString("HH:mm:ss");
    entry.text = text;
    appendEntry(entry);
}

void MainWindow::appendDataLog(bool isRx, const QByteArray &data)
{
    LogEntry entry;
    entry.type = isRx ? LogEntry::Rx : LogEntry::Tx;
    entry.time = QDateTime::currentDateTime().toString("HH:mm:ss");
    entry.data = data;
    appendEntry(entry);
}

void MainWindow::renderLog()
{
    ui->log_output_browser->clear();
    for (const LogEntry &entry : m_logEntries) {
        if (isEntryVisible(entry)) {
            appendEntryToBrowser(entry);
        }
    }
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
            appendDataLog(true, line);
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
    appendDataLog(true, line);
}

void MainWindow::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        appendLog(QString("串口错误：%1").arg(m_serial->errorString()));
        updateStatusLabel();
    }
}
