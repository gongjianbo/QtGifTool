#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextCodec>
#include <QFileDialog>
#include <QPixmap>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initOperate();
    initTest();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initOperate()
{
    //选择文件
    connect(ui->btnSelect,&QPushButton::clicked,[=]{
        const QString path=QFileDialog::getOpenFileName(this,"select gif","","gif(*.gif);");
        if(path.isEmpty())
            return;
        ui->editLoad->setText(path);
        ui->editSave->setText(path+"saveas.gif");
        manager.loadGif(path);
    });

    //导入文件
    connect(ui->btnLoad,&QPushButton::clicked,[=]{
        const QString path=ui->editLoad->text();
        if(path.isEmpty())
            return;
        manager.loadGif(path);
    });
    //导出文件
    connect(ui->btnSave,&QPushButton::clicked,[=]{
        const QString path=ui->editSave->text();
        if(path.isEmpty())
            return;
        manager.saveGif(path);
    });
    //加载完毕
    connect(&manager,&GifManager::gifChanged,[=]{
        if(manager.getFrameCount()>0){
            const QImage img=manager.getCurrent(0);
            ui->labelImage->setPixmap(QPixmap::fromImage(img));
            ui->gifSlider->setRange(0,manager.getFrameCount()-1);
            //测试
            ui->spinCenterX->setValue(img.width()/2);
            ui->spinFrameBegin->setValue(1);
            ui->spinFrameEnd->setValue(manager.getFrameCount());
        }else{
            ui->labelImage->clear();
            ui->gifSlider->setRange(0,0);
        }
        ui->labelMessage->setText(QString("1/%1").arg(manager.getFrameCount()));
        ui->gifSlider->setValue(0);
    });
    connect(&manager,&GifManager::currentIndexChanged,[=](int index){
        ui->labelMessage->setText(QString("%1/%2").arg(index+1).arg(manager.getFrameCount()));
        if(index!=ui->gifSlider->value())
            ui->gifSlider->setValue(index);
    });
    //滚动条
    connect(ui->gifSlider,&QSlider::valueChanged,[this](int value){
        ui->labelImage->setPixmap(QPixmap::fromImage(manager.getCurrent(value)));
    });
    //第一帧
    connect(ui->btnFirst,&QPushButton::clicked,[=]{
        ui->labelImage->setPixmap(QPixmap::fromImage(manager.getCurrent(0)));
    });
    //前一帧
    connect(ui->btnPrev,&QPushButton::clicked,[=]{
        ui->labelImage->setPixmap(QPixmap::fromImage(manager.getPrev()));
    });
    //下一帧
    connect(ui->btnNext,&QPushButton::clicked,[=]{
        ui->labelImage->setPixmap(QPixmap::fromImage(manager.getNext()));
    });
}

void MainWindow::initTest()
{
    //测试功能，后期会修改掉
    //预览
    connect(ui->btnPreview,&QPushButton::clicked,[this]{
        QImage img=QImage(manager.getCurrent().size(),QImage::Format_ARGB32);
        QPainter painter(&img);
        if(painter.isActive())
        {
            QFont font=ui->fontComboBox->font();
            font.setPixelSize(ui->spinFontPx->value());
            painter.setFont(font);

            const int center_x=ui->spinCenterX->value();
            const int center_y=ui->spinCenterY->value();
            const QString text=ui->editText->text();
            QFontMetrics fm=painter.fontMetrics();
            QRect text_rect(0,0,fm.width(text),fm.height());
            text_rect.moveCenter(QPoint(center_x,center_y));

            painter.drawImage(0,0,manager.getCurrent());
            painter.drawText(text_rect,text);

            ui->labelImage->setPixmap(QPixmap::fromImage(img));
        }
    });
    //修改
    connect(ui->btnChange,&QPushButton::clicked,[this]{
        QImage img=QImage(manager.getCurrent().size(),QImage::Format_ARGB32);
        QPainter painter(&img);
        if(painter.isActive())
        {
            QFont font=ui->fontComboBox->font();
            font.setPixelSize(ui->spinFontPx->value());
            painter.setFont(font);

            const int center_x=ui->spinCenterX->value();
            const int center_y=ui->spinCenterY->value();
            const QString text=ui->editText->text();
            QFontMetrics fm=painter.fontMetrics();
            QRect text_rect(0,0,fm.width(text),fm.height());
            text_rect.moveCenter(QPoint(center_x,center_y));

            const int begin=ui->spinFrameBegin->value()-1;
            const int end=ui->spinFrameEnd->value()-1;
            if(begin>end||begin<0||end>manager.getFrameCount()-1)
                return;
            for(int index=begin;index<=end;index++)
            {
                img.fill(Qt::transparent);
                painter.drawImage(0,0,manager.getImage(index));
                painter.drawText(text_rect,text);
                manager.setImage(img,index);
            }
            ui->labelImage->setPixmap(QPixmap::fromImage(manager.getCurrent(begin)));
        }
    });
}

