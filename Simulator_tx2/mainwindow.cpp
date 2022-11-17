#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "spidev.h"
#include "gpio_api.h"

#include <QTimer>
#include <algorithm>

//------Needed for SPI port------
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
//-------------------------------

#include <QThread>
#include <QtConcurrent>

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
    ui->widget->yAxis->setRange(4690000,3400057);
    ui->widget->yAxis->setRangeReversed(true);
    ui->widget->replot();

//    time_t test_timer = time(NULL);
//    struct tm* t = localtime(&test_timer);
//    qDebug() << "current time"<< t->tm_year << t->tm_yday << t->tm_yday << t->tm_hour << t->tm_min << t->tm_sec;

//    QDateTime dateTime = QDateTime::currentDateTime();
//    QString time_format = "yyyy-MM-dd HH:mm:ss";
//    QString print_time = dateTime.toString(time_format);
//    qDebug() << print_time;
//    qDebug() << ".........";


    QSpinBox *test = new QSpinBox();
    ui->spinBox->setRange(4200000,48000000);
    ui->spinBox->setSingleStep(10000);
    ui->spinBox->setValue(4400000);
    connect(ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(on_spinBox_valueChanged(int)));

    //    ui->textBrowser_2->setText(test);

    //QTimer *timer = new QTimer();
    //timer->setInterval(100);
    connect(timer,SIGNAL(timeout()),this, SLOT(qwt_update()));

    ui->pushButton_2->setCheckable(true);
    ui->pushButton_2->setText("STOP");
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

    QFileInfo info(file);
    fileinfo = info.completeSuffix();
    qDebug() << fileinfo;

    if (fileinfo == "json"){
        qDebug() << "Loda Json";

        Load_json_file(file);

        timer->start(90);
    }else if(fileinfo == "csv"){
        qDebug() << "Load CSV";

        Load_csv_file(file);

        timer->start(90);
    } else {
        qDebug() << "not supported";
    }
    //peakdet(IMSData_a,IMSData_x,8000000, 8000000, 8000000);
}
void MainWindow::Load_json_file(QString File){

    IMSData_a.clear();

    MaxPeakThreshold = 7000000;
    MinPeakThreshold = 8000000;
    peak_distance = 3900;

    ArraySize = 10000;

    QFile loadFile(File);

    if(!loadFile.open(QIODevice::ReadOnly)){
       qWarning("Could not open file to read");
    }
    else {
       qDebug() << "Open Success";
    }


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

    if(jsonObj.contains("ims")){
        QJsonArray subArray = jsonObj.value("ims").toArray();
        for(int i=0; i<subArray.size(); i++){
            //qDebug() << i << "value is : " << subArray.at(i).toString();
            QString convert = subArray.at(i).toString();
            IMSData_a.append(convert.split(" ")[0].toInt()*-1);
//            IMSData_a.append(subArray.at(i).toInt());
            IMSData_x.append(i);
        }
        qDebug() << "Ims data a = " << IMSData_a[1];
        peak_detect(IMSData_a, MinPeakThreshold, MaxPeakThreshold);
        peak_cnt = peaks_x[0];

    }

}

void MainWindow::Load_csv_file(QString File){

    IMSData_a.clear();
    DownSampling_data.clear();

    ui->widget->yAxis->setRange(4200000,3300000);

    MaxPeakThreshold = 3300000;
    MinPeakThreshold = 4000000;
    peak_distance = 450;

    ArraySize = 1500;


    QFile loadFile(File);

    if(!loadFile.open(QIODevice::ReadOnly)){
       qWarning("Could not open file to read");
    }
    else {
       qDebug() << "Open Success";
    }

    while (!loadFile.atEnd()) {
       QByteArray loadData = loadFile.readLine();
       loadData = loadData.split('\n')[0];
       IMSData_a.append(loadData.split(' ')[1].toInt()*-1);
       IMSData_x.append(loadData.split(',')[0].toInt());
    }
    int test = 0;
    for(int i =0; i < IMSData_a.size(); i++){
        if(i % 10 == 0 && i > 0){
           //qDebug() << "i = " << i;
           test = test / 10;
           DownSampling_data << test;
           test = 0;
           test += IMSData_a[i];
        }
        test += IMSData_a[i];
    }
    MinPeakThreshold = 4400000;
    qDebug() << "DownSampling_data size (8129) = " << DownSampling_data.size();
    qDebug() << "CSV2";
    //peak_detect(IMSData_a, MinPeakThreshold, MaxPeakThreshold);
    //qDebug() << DownSampling_data.at(1);
    peak_detect(DownSampling_data, MinPeakThreshold, MaxPeakThreshold);
    peak_cnt = peaks_x[0];
}

void MainWindow::peak_detect(QVector<double> data, double thresholdMin, double thresholdMax)
{
    //qDebug() << "data.size() = " << data.size();
    //qDebug() << "thr = " << MinPeakThreshold << MaxPeakThreshold ;
    //qDebug() << data << data[2];
    for(int i=1; i < data.size()-1; i++){
        if(data[i] < thresholdMin){
            //qDebug() << data[i-1] << data[i];
            if(data[i-1] > data[i]){
                if(data[i] < data[i + 1]){
                    peaks_y << (double) data[i];
                    peaks_x << (double) i;

                }

            }
        }
    }
//    qDebug() << "X size = " << peaks_x.size() << "Y size = " << peaks_y.size();
//    qDebug() << "X peak = " << peaks_x<< "Y peak = " << peaks_y;
}

void MainWindow::show_TextLabel(double x, double y)
{
    textLabel = new QCPItemText(ui->widget);
    textLabel->position->setCoords(x, y+30000); // place position at center/top of axis rect
    textLabel->setText(Material);
    textLabel->setColor(QColor(Qt::white));
    textLabel->setFont(QFont(font().family(), 15, QFont::Bold)); // make font a bit larger
    textLabel->setPen(QPen(Qt::red)); // show black border around text
    textLabel->setBrush(QColor(Qt::red));
}

void MainWindow::show_line(int x, int y)
{
    QCPItemLine *DrawLine = new QCPItemLine(ui->widget);
    DrawLine->setPen(QColor(Qt::red));
    DrawLine->start->setCoords(0,y);
    DrawLine->end->setCoords(x,y);

}

bool MainWindow::compare(double x, double y){
    return x < y;
}

void MainWindow::MobilityDetection(){
    for (int i =0 ; i < peaks_x.size(); i++){
        //if(abs(peaks_x[i+1] - peaks_x[0]) >= 650){
        if(peaks_x[i] > 8 && peaks_x[i] < 10){
            //qDebug() << "peaks_x[i]" << peaks_x[i] << "peaks_y[i]" << peaks_y[i];
            Material = "Detection TNT";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }else if(peaks_x[i] > 10 && peaks_x[i] < 12){
            Material = "Detection RDX";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }
        else if(peaks_x[i] > 12 && peaks_x[i] < 15){
            Material = "Detection NG";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }
        else if(peaks_x[i] > 18 && peaks_x[i] < 20){
            Material = "Detection PETN";
            Detection_x = peaks_x[i];
            Detection_y = peaks_y[i];
        }else{
            ui->widget->clearItems();
        }
    }
}

void MainWindow::variable_clear(){
    aa.clear();
    xx.clear();
    peaks_x.clear();
    peaks_y.clear();
    MaxPeaks.clear();
    Detection_x = 0;
    Detection_y = 0;

    Material = "......";
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
#if 0   //using original data
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

        if(peaks_x[i] <= 730){
            if(peaks_x[i] < 500){
                MaxPeaks << 4000000;
            }else{
                MaxPeaks << peaks_y[i];
            }
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

    if(MaxPeaks.size() == 0){
        qDebug() << "Max Peaks size is empty";
       for (int i =0 ; i < peaks_x.size()-1; i++){
           if(peaks_x[i] > 730){
               MaxPeaks << peaks_y[i];
               break;
           }
       }
    }

    auto max = std::minmax_element(MaxPeaks.begin(), MaxPeaks.end());
    int max_idx = std::distance(MaxPeaks.begin(), max.first);
    qDebug() << "MaxPeaks" << MaxPeaks;
    qDebug() << "peaks_x" << peaks_x << "peaks_y" << peaks_y;
    qDebug() << "Max_idx" << max_idx;
    qDebug() << "peaks_x[max_idx]" << peaks_x[max_idx] << "peaks_y[max_idx]" << peaks_y[max_idx];
    peak_cnt = peak_cnt + peaks_x[max_idx];


#endif

#if 1   // using Down sampling data
    for(int j=0; j < 200; j++){
        if(j+peak_cnt >= DownSampling_data.size()){
            qDebug() << "repeat";
            qDebug() << "if" << j+peak_cnt << "END" << DownSampling_data.size();
            peak_detect(DownSampling_data, MinPeakThreshold, MaxPeakThreshold);
            peak_cnt = peaks_x[0];
            timer->stop();
        }
        aa << (double) DownSampling_data[j+peak_cnt];
        xx << (double) j;
    }

    peaks_x.clear();
    peaks_y.clear();

    peak_detect(aa, MinPeakThreshold, MaxPeakThreshold);

    MobilityDetection();

    for(int i=0; i < peaks_x.size(); i++){
        if(peaks_x[i] > 60){
            peak_cnt = peak_cnt + peaks_x[i];
            break;
        }
    }

#endif

    //peak_detect(aa, 8000000, 7000000);
    //peak_detect(aa, 3900000, 3300000);

    //qDebug() << "update" << IMSData_a.size();
    ui->widget->graph(0)->setData(xx,aa);
    ui->widget->graph(1)->setData(peaks_x,peaks_y);
    //ui->widget->yAxis->rescale(true);
    ui->widget->xAxis->setRange(0,50);
    ui->widget->yAxis->setRange(4690000,3800000);
    //ui->widget->xAxis->setRange(0,xx.size());
    //ui->widget->xAxis->setRange(0,peak_distance);
    ui->textBrowser_2->setText(Material);
    ui->textBrowser_2->setTextColor(QColor(Qt::red));

    show_TextLabel(Detection_x, Detection_y);
    show_line(50,MinPeakThreshold);

    ui->widget->update();
    ui->widget->replot();

    variable_clear();

    //timer->stop();
}

void MainWindow::on_pushButton_2_toggled(bool checked)
{
    if(!checked){
        ui->pushButton_2->setStyleSheet("background-color:green;");
        ui->pushButton_2->setText("STOP");
        timer->start(90);
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

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    MinPeakThreshold = arg1;
    //ui->textBrowser_2->setText(QString::number(arg1));
}

int MainWindow::on_pushButton_3_clicked()
{

    ui->textBrowser->setText("Open Sensor Device");
    QProgressDialog qdialog;
    qdialog.setLabelText(QString("sensing ..."));

    QFutureWatcher<void> watcher;
    connect(&watcher, &QFutureWatcher<void>::finished, &qdialog, &QProgressDialog::reset);
    connect(&qdialog, &QProgressDialog::canceled, &watcher, &QFutureWatcher<void>::cancel);
    connect(&watcher, &QFutureWatcher<void>::progressRangeChanged, &qdialog, &QProgressDialog::setRange);
    connect(&watcher, &QFutureWatcher<void>::progressValueChanged, &qdialog, &QProgressDialog::setValue);

    QFuture<void> result = QtConcurrent::run([=]{ openDevice(); });

    watcher.setFuture(result);

    qdialog.move(QApplication::desktop()->screen()->rect().center() - qdialog.rect().center());

    qdialog.exec();

    watcher.waitForFinished();

}

int MainWindow::openDevice() {

    qDebug() << __func__ << QThread::currentThread();

    QString date_format = "yyyy-MM-dd";
    date = QDate::currentDate().toString(date_format);
    Time = QTime::currentTime().toString();

    int ret = 0;
    int fd;
    qDebug() << "start spi device";
    // 프로그램 우선순위 높임
    rt_process();

    qDebug() << "1";
    // GPIO 설정, SPI 초기화
    gpio_sync_init();
    gpio_drdy_init();
    qDebug() << "2";
    fd = open(device, O_RDWR);
    qDebug() << "3 , fd = " << fd;
    if (fd < 0){
        pabort("can't open device");
    }
    else if (fd == 30){
        pabort("permission denine");
        return 0;
    }

    //Mode setting
    mode |= SPI_CPHA;

    // spi mode
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
       pabort("can't set spi mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1)
       pabort("can't get spi mode");

    // bits per word
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
       pabort("can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
       pabort("can't get bits per word");

    // max speed hz
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
       pabort("can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
       pabort("can't get max speed hz");

    qDebug() << "4";
    //usleep(1000);
    transfer(fd);
    ::close(fd);      // spi close

    qDebug() << "5";
    data_output(date,Time);
    filter_output(date,Time);
    Load_csv_file("/home/keti/projects/TNT_8.csv");
    //data_send();

    qDebug() << "6";
    gpio_close(-1);
    gpio_close(-1);

    #if ( GPIO_CHECK_ENABLE == 1 )
       gpio_close(gpio_chk_fd);
    #endif

    qDebug() << "finish spi device";
    qDebug() << "ret" << ret;
    return ret;
}
