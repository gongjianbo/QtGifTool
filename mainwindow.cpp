#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "gif_lib.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //测试接口是否可以调用
    qDebug()<<GifErrorString(D_GIF_ERR_NOT_GIF_FILE);
}

MainWindow::~MainWindow()
{
    delete ui;
}

