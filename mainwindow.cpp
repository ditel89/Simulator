#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // -----------------Using QCustom Widget---------------------
    ui->widget->addGraph();
    ui->widget->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    ui->widget->addGraph();
    ui->widget->graph(1)->setPen(QPen(QColor(255,110,40)));
    ui->widget->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
    ui->widget->graph(1)->setLineStyle(QCPGraph::lsNone);

    ui->widget->yAxis->grid()->setVisible(false);
    ui->widget->yAxis->setVisible(false);

    // give the axes some labels:
    ui->widget->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag | QCP::iSelectPlottables);
    ui->widget->xAxis->setLabel("x");
    ui->widget->yAxis->setLabel("y");

    ui->widget->xAxis->setRange(0,5000);
    ui->widget->yAxis->setRangeReversed(true);
    ui->widget->replot();

    //QTimer *timer = new QTimer();
    timer->setInterval(100);
    connect(timer,SIGNAL(timeout()),this, SLOT(qwt_update()));

    ui->pushButton_2->setCheckable(true);
    ui->pushButton_2->setStyleSheet("background-color:green;");
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(on_pushButton_2_toggled()));

    connect(ui->widget, SIGNAL(mouseMove(QMouseEvent*)), this,SLOT(showPointToolTip(QMouseEvent*)));

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    //    QString dir = QFileDialog::getExistingDirectory(this,
    //                                                    "파일선택",
    //                                                    QDir::currentPath(),
    //                                                    QFileDialog::ShowDirsOnly);
    //    qDebug()<<dir;
        QString file = QFileDialog::getOpenFileName(this,"파일선택","./","Files(*.*)");

        qDebug()<<file;

        ui->textBrowser->setText(file);

        //QFile loadFile(QStringLiteral("../TNT_test_1.json"));
        QFile loadFile(file);

        QFileInfo info(file);
        fileinfo = info.completeSuffix();
        qDebug() << fileinfo;

        if(!loadFile.open(QIODevice::ReadOnly)){
            qWarning("Could not open file to read");
        }
        else {
            qDebug() << "Open Success";
        }


        if (fileinfo == "json"){

            QByteArray loadData = loadFile.readAll();
            loadFile.close();

            QJsonParseError json_error;
            QJsonDocument loadDoc(QJsonDocument::fromJson(loadData, &json_error));

            if(json_error.error != QJsonParseError::NoError){
                qDebug() << "json error!!";
                return;
            }

            QJsonObject jsonObj = loadDoc.object();
            QStringList keys = jsonObj.keys();

            for(int i=0; i<keys.size(); i++){
                qDebug() << "key" << i << "is : " << keys.at(i);
            }

            IMSData_a.clear();

            MaxPeakThreshold = 7000000;
            MinPeakThreshold = 8000000;
            peak_distance = 3900;

            ArraySize = 10000;

            if(jsonObj.contains("ims")){
                QJsonArray subArray = jsonObj.value("ims").toArray();
                for(int i=0; i<subArray.size(); i++){
                    //qDebug() << i << "value is : " << subArray.at(i).toString();
                    QString convert = subArray.at(i).toString();
                    IMSData_a.append(convert.split(" ")[0].toInt()*-1);
        //            IMSData_a.append(subArray.at(i).toInt());
                    IMSData_x.append(i);
                }
                peak_detect(IMSData_a, MinPeakThreshold, MaxPeakThreshold);
                peak_cnt = peaks_x[0];

            }
        } else if(fileinfo == "csv"){
           qDebug() << "CSV";
           IMSData_a.clear();

           ui->widget->yAxis->setRange(4200000,3300000);

           MaxPeakThreshold = 3300000;
           MinPeakThreshold = 4000000;
           peak_distance = 450;

           ArraySize = 1500;

           while (!loadFile.atEnd()) {
               QByteArray loadData = loadFile.readLine();
               IMSData_a.append(loadData.split(',')[1].toInt()*-1);
               IMSData_x.append(loadData.split(',')[0].toInt());
           }

           qDebug() << "CSV2";
           peak_detect(IMSData_a, MinPeakThreshold, MaxPeakThreshold);
           peak_cnt = peaks_x[0];

        } else {
            qDebug() << "not supported";

        }


        //peakdet(IMSData_a,IMSData_x,8000000, 8000000, 8000000);
        timer->start();
}

void MainWindow::peak_detect(QVector<double> data, double thresholdMin, double thresholdMax){

    if(thresholdMin != 0){
        for(int i=1; i < data.size()-1; i++){
            if(data[i] < thresholdMin && data[i] >= thresholdMax){
                if(data[i - 1] > data[i] && data[i] < data[i + 1]){
                    peaks_y << (double) data[i];
                    peaks_x << (double) i;
                }
            }
//            if(data[i-1] <= data[i] &&
//                    data[i] >= data[i+1] &&
//                    data[i] >= thresholdMax &&
//                    data[i] < thresholdMin){
//                peaks_y << (double) data[i];
//                peaks_x << (double) i;
//            }
        }
    }
    else{
        for(int i=1; i < data.size()-1; i++){
            if(data[i - 1] > data[i] && data[i] < data[i + 1]){
                peaks_y << (double) data[i];
                peaks_x << (double) i;
            }
        }
    }
    //qDebug() << "X size = " << peaks_x.size() << "Y size = " << peaks_y.size();
    //qDebug() << "X peak = " << peaks_x<< "Y peak = " << peaks_y;
}

void MainWindow::show_TextLabel(double x, double y){
    textLabel = new QCPItemText(ui->widget);
    textLabel->position->setCoords(x, y+30000); // place position at center/top of axis rect
    textLabel->setText(Material);
    textLabel->setColor(QColor(Qt::white));
    textLabel->setFont(QFont(font().family(), 15, QFont::Bold)); // make font a bit larger
    textLabel->setPen(QPen(Qt::red)); // show black border around text
    textLabel->setBrush(QColor(Qt::red));
}

bool MainWindow::compare(double x, double y){
    return x < y;
}

void MainWindow::qwt_update()
{
#if 0
    for(int j=0; j < ArraySize; j++){
        if(j+(cnt*ArraySize) == IMSData_a.size()){
            qDebug() << "repeat";
            qDebug() << "if" << j+(cnt*ArraySize) << "END" << IMSData_a.size();
            cnt = 0;
        }
        //IMSData_aa[j] = IMSData_a[j+(cnt*ArraySize)];
        //IMSData_xx[j] = j;
        aa << (double) IMSData_a[j+(cnt*ArraySize)];
        xx << (double) j;
    }

    cnt++;

    peak_detect(aa, 3900000, 3300000);
#endif
#if 1   //test freq using peak detect
    for(int j=0; j < ArraySize; j++){
        if(j+peak_cnt >= IMSData_a.size()){
            qDebug() << "repeat";
            qDebug() << "if" << j+peak_cnt << "END" << IMSData_a.size();
            peak_detect(IMSData_a, MinPeakThreshold, MaxPeakThreshold);
            peak_cnt = peaks_x[0];
            timer->stop();
        }
        //IMSData_aa[j] = IMSData_a[j+(cnt*ArraySize)];
        //IMSData_xx[j] = j;
        if(abs(IMSData_a[(j+peak_cnt)] -
               (IMSData_a[(j+peak_cnt)-1] - IMSData_a[(j+peak_cnt)-2])) > 80000){
            aa << (double) (IMSData_a[(j+peak_cnt)-1] + IMSData_a[(j+peak_cnt)-2]) / 2;
        }else {
            aa << (double) IMSData_a[j+peak_cnt];
        }
        //aa << (double) IMSData_a[j+peak_cnt];
        xx << (double) j;
        //xx << (double) j-687;
    }
//    qDebug() << "2";
//    qDebug() << "peaks_x[peak_cnt]" << peak_cnt;
//    qDebug() << "origin" << IMSData_a[peak_cnt] << "aa" << aa[0];

    peaks_x.clear();
    peaks_y.clear();

    peak_detect(aa, MinPeakThreshold, MaxPeakThreshold);
//    qDebug() << "find peak";
//    qDebug() << "aa size" << aa.size();
    //qDebug() << "peak cnt" << peak_cnt;
    for (int i =0 ; i < peaks_x.size()-1; i++){
        //if(abs(peaks_x[i+1] - peaks_x[0]) >= 650){        
        if(peaks_x[i] > 80 && peaks_x[i] < 110){
            //qDebug() << "peaks_x[i]" << peaks_x[i] << "peaks_y[i]" << peaks_y[i];
            Material = "Detection TNT";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }else if(peaks_x[i] > 110 && peaks_x[i] < 125){
            Material = "Detection RDX";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }
        else if(peaks_x[i] > 128 && peaks_x[i] < 160){
            Material = "Detection NG";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }
        else if(peaks_x[i] > 180 && peaks_x[i] < 200){
            Material = "Detection PETN";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }else{
            ui->widget->clearItems();
        }

        if(peaks_x[i] <= 780){
            MaxPeaks << peaks_y[i];
//        if(peaks_x[i] >= peak_distance){
//            if(peaks_y[i] < peaks_y[i+1]){
//                peak_cnt = peak_cnt + peaks_x[i+1];
//            } else {
//                peak_cnt = peak_cnt + peaks_x[i];
//            }
//            peak_cnt = peak_cnt + peaks_x[i];
            //qDebug() << "peaks_x[i]" << peaks_x[i] << "peaks_y[i]" << peaks_y[i];
//        else if(peaks_x[i] < peak_distance && peaks_x[i+1] < peak_distance){
//            int c = compare(peaks_y[i], peaks_y[i+1]);
//            if(c == 1){

//            }
        }
    }
    auto max = std::minmax_element(MaxPeaks.begin(), MaxPeaks.end());
    int max_idx = std::distance(MaxPeaks.begin(), max.first);
    qDebug() << "MaxPeaks" << MaxPeaks;
    qDebug() << "peaks_x" << peaks_x << "peaks_y" << peaks_y;
    qDebug() << "Max_idx" << max_idx;
    peak_cnt = peak_cnt + peaks_x[max_idx];


#endif

    //peak_detect(aa, 8000000, 7000000);
    //peak_detect(aa, 3900000, 3300000);

    //qDebug() << "update" << IMSData_a.size();
    ui->widget->graph(0)->setData(xx,aa);
    ui->widget->graph(1)->setData(peaks_x,peaks_y);
    ui->widget->yAxis->rescale(true);
    //ui->widget->xAxis->setRange(0,xx.size());
    ui->widget->xAxis->setRange(0,peak_distance);

    show_TextLabel(Detection_x, Detection_y);

    ui->widget->update();
    ui->widget->replot();
    aa.clear();
    xx.clear();
    peaks_x.clear();
    peaks_y.clear();
    MaxPeaks.clear();
    Detection_x = 0;
    Detection_y = 0;
    ui->widget->clearItems();

    //timer->stop();
}

void MainWindow::on_pushButton_2_toggled(bool checked)
{
    if(!checked){
        ui->pushButton_2->setStyleSheet("background-color:green;");
        ui->pushButton_2->setText("STOP");
        timer->start();
    }
    else{
        ui->pushButton_2->setStyleSheet("background-color:red;");
        ui->pushButton_2->setText("START");
        timer->stop();
    }
}

void MainWindow::showPointToolTip(QMouseEvent *event)
{

    int x = ui->widget->xAxis->pixelToCoord(event->pos().x());
    int y = ui->widget->yAxis->pixelToCoord(event->pos().y());

    setToolTip(QString("%1 , %2").arg(x).arg(y));

}

