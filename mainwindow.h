#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_refresh_pushButton_clicked();
    void on_refresh_pushButton_2_clicked();
    void on_refresh_pushButton_3_clicked();
    void on_input_lineEdit_returnPressed();
    void readSerialData();
    void onSerialError(QSerialPort::SerialPortError error);
    void flushRxBuffer();

private:
    void refreshPorts();
    bool ensurePortOpen();
    void appendLog(const QString &text);

    Ui::MainWindow *ui;
    QSerialPort *m_serial;
    QByteArray m_rxBuffer;
    QTimer m_rxFlushTimer;
};
#endif // MAINWINDOW_H
