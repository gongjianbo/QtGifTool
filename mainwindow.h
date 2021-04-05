#pragma once
#include <QMainWindow>
#include <QImage>
#include <QFutureWatcher>


struct GifInfo
{
    int width=0;
    int height=0;
    int interval=0;
    int colors=0;
    int loopcount=0;
    QList<QImage> images;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    //解析完后通过信号槽发到主线程展示
    void decodeGifFinished(const GifInfo &info);

public slots:
    //翻页
    void displayPrev();
    void displayNext();
    //点击读取Gif-DGifSlurp
    void decodeGif();
    //点击读取Gif-流式操作
    void decodeGifStream();
    //点击选择图片列表
    void selectImages();
    //点击生成Gif-EGifSpew
    void encodeGif();

private:
    Ui::MainWindow *ui;
    //gif图片帧列表
    QList<QImage> imgList;
    //
    QFutureWatcher<void> taskWatcher;
};
