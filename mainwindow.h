#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
#include <QList>
#include <QMainWindow>
#include <QSerialPort>
#include <QString>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct LogEntry {
    enum Type { System, Rx, Tx };
    Type type = System;
    QString time;
    QString text;
    QByteArray data;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_refresh_pushButton_clicked();
    void on_open_pushButton_clicked();
    void on_send_pushButton_clicked();
    void on_clear_pushButton_clicked();
    void on_hex_checkBox_toggled(bool checked);
    void on_rx_checkBox_toggled(bool checked);
    void on_tx_checkBox_toggled(bool checked);
    void on_baud_comboBox_currentIndexChanged(int index);
    void on_data_bits_comboBox_currentIndexChanged(int index);
    void on_parity_comboBox_currentIndexChanged(int index);
    void on_stop_bits_comboBox_currentIndexChanged(int index);
    void on_input_lineEdit_returnPressed();
    void readSerialData();
    void onSerialError(QSerialPort::SerialPortError error);
    void flushRxBuffer();

private:
    void refreshPorts();
    bool ensurePortOpen();
    void closePort();
    void applySerialParams();
    void sendData();
    void updateStatusLabel();
    void appendLog(const QString &text);
    void appendDataLog(bool isRx, const QByteArray &data);
    void appendEntry(const LogEntry &entry);
    void appendEntryToBrowser(const LogEntry &entry);
    bool isEntryVisible(const LogEntry &entry) const;
    void renderLog();

    Ui::MainWindow *ui;
    QSerialPort *m_serial;
    QByteArray m_rxBuffer;
    QTimer m_rxFlushTimer;
    QList<LogEntry> m_logEntries;
};
#endif // MAINWINDOW_H
