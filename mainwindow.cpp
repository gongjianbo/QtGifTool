#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnFile,&QPushButton::clicked,[=]{
        const QString path=QFileDialog::getOpenFileName(this,"select gif","","gif(*.gif);");
        if(path.isEmpty())
            return;
        manager.loadGif(path);
    });

    /*fipImage image;
    image.load("F:/Src/code.png");
    if(!image.isValid())
        exit(-1);

    fipImage saveimage(image);
    //bpp参数位图深度，直接构造不好用，还是拷贝构造一个吧
    //fipImage saveimage(FIT_BITMAP,image.getWidth(),image.getHeight(),8);
    if(!saveimage.isValid())
        exit(-2);

    //或者用accessPixels
    for(unsigned int row=0;row<image.getHeight();row++)
    {
        const unsigned char *line=image.getScanLine(row);
        unsigned char *saveline=saveimage.getScanLine(row);
        for(unsigned int col=0;col<image.getWidth();col++)
        {
            saveline[col*4+0]=line[col*4+2];
            saveline[col*4+1]=line[col*4+1];
            saveline[col*4+2]=line[col*4+0];
            saveline[col*4+3]=line[col*4+3];
        }
    }

    saveimage.save(FIF_PNG,"F:/Src/code3.png",PNG_DEFAULT);*/
}

MainWindow::~MainWindow()
{
    delete ui;
}

