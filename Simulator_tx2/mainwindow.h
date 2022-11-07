#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <qcustomplot.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void peak_detect(QVector<double> data, double thresholdMin, double thresholdMax);
    void show_TextLabel(double x, double y);
    void show_line(int x, int y);
    void MobilityDetection();
    void variable_clear();
    void Load_json_file(QString File);
    void Load_csv_file(QString File);

    //#define DownSampling

    QString date;
    QString Time;

private slots:
    void on_pushButton_clicked();
    void qwt_update();

    void on_pushButton_2_toggled(bool checked);
    void showPointToolTip(QMouseEvent *event);
    bool compare(double x, double y);

    void on_spinBox_valueChanged(int arg1);

    int on_pushButton_3_clicked();

private:
    Ui::MainWindow *ui;


    //QString IMSData;
    QStringList IMSData;
    QVector<double> IMSData_a;
    QVector<double> IMSData_x;

    QVector<double> DownSampling_data;
    QTimer *timer = new QTimer();

    QCPItemText *textLabel;

    int peak_cnt = 0;
    int cnt = 0;

    int ArraySize = 5000;

    int peak_distance = 0;
    int MaxPeak = 0;

    int MaxPeakThreshold = 0;
    int MinPeakThreshold = 0;

    int Detection_x;
    int Detection_y;


    double IMSData_xx[5000];
    double IMSData_aa[5000];
    //QVector<double> IMSData_xx;
    //QVector<double> IMSData_aa;
    QVector<double> xx;
    QVector<double> aa;
    QString fileinfo;
    QString Material;

    QVector<int> MaxPeaks;
    QVector<double> peaks_y;
    QVector<double> peaks_x;

};
#endif // MAINWINDOW_H
