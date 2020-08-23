#include "GifManager.h"

#include "gif.h"

#include <string>
#include <QTextCodec>
#include <QImageReader>
#include <QFuture>
#include <QtConcurrentRun>
#include <QThread>
#include <QDebug>

GifManager::GifManager(QObject *parent) : QObject(parent)
{
    //读取结束
    connect(&readWatcher,&QFutureWatcher<QList<GifFrame>>::finished,
            this,[this]{
        gifList=readWatcher.result();
        qDebug()<<"load end:"<<gifList.count();
        emit gifChanged();
        setCurrentIndex(0);
    });
    //写入结束
    connect(&writeWatcher,&QFutureWatcher<void>::finished,
            this,[this]{
        qDebug()<<"save end:";
    });
    }

    GifManager::~GifManager()
    {
        if(!readWatcher.isFinished())
        readWatcher.waitForFinished();
        if(!writeWatcher.isFinished())
        writeWatcher.waitForFinished();
    }

    void GifManager::loadGif(const QString &path)
    {
        if(!readWatcher.isFinished())
        return;
        QFuture<QList<GifFrame>> future=QtConcurrent::run([path](){
            qDebug()<<"load:"<<path;
            QImageReader reader(path,"gif");
            QList<GifFrame> data;
            if(reader.imageCount()>0)
            {
                reader.jumpToImage(0);
                for(int i=0;i<reader.imageCount();i++)
                {
                    //qDebug()<<"read"<<i<<reader.nextImageDelay();
                    data.push_back(GifFrame{i,reader.read()});
                    reader.jumpToNextImage();
                    QThread::msleep(0);
                }
            }
            return data;
        });
        readWatcher.setFuture(future);
    }

    void GifManager::saveGif(const QString &path)
    {
        if(!writeWatcher.isFinished())
        return;
        QFuture<void> future=QtConcurrent::run([path,this](){
            qDebug()<<"save:"<<path;
            const QList<GifFrame> gif_data=this->gifList;//copy
            if(gif_data.isEmpty())
            return;

            const QByteArray file_path=path.toLocal8Bit();
            //测试中文是否正常
            printf("save: %s \n",file_path.constData());
            fflush(stdout);

            //经测试这个gif库的delay参数有问题，设置10勉强能用
            //有空找找其他的库
            GifWriter g;
            GifBegin(&g, file_path.constData(),gif_data.first().image.width() , gif_data.first().image.height(), 10);
            for(int i=0;i<gif_data.count();i++)
            {
                QImage img=gif_data.at(i).image;
                if(img.format()!=QImage::Format_ARGB32)
                img.convertTo(QImage::Format_ARGB32);
                //qDebug()<<"write"<<i<<gif_data.at(i).delay;
                GifWriteFrame(&g, img.constBits(), img.width(), img.height(), 10);
            }
            GifEnd(&g);
        });
        writeWatcher.setFuture(future);
    }

    int GifManager::getFrameCount() const
    {
        return gifList.count();
    }

    void GifManager::setCurrentIndex(int index)
    {
        currentIndex=index;
        if(currentIndex<0||currentIndex>=gifList.count())
        currentIndex=0;
        emit currentIndexChanged(currentIndex);
    }

    int GifManager::getCurrentIndex() const
    {
        return currentIndex;
    }

    QImage GifManager::getImage(int index) const
    {
        if(indexValid(index))
        return gifList.at(index).image;
        else if(indexValid(getCurrentIndex()))
        return gifList.at(getCurrentIndex()).image;
        return QImage();
    }

    QImage GifManager::getCurrent(int index)
    {
        //无效的index就用默认的
        if(indexValid(index))
        setCurrentIndex(index);
        const int cur_index=getCurrentIndex();//为什么set时不直接返回呢
        if(indexValid(cur_index))
        return gifList.at(cur_index).image;
        return QImage();
    }

    QImage GifManager::getPrev()
    {
        return getCurrent(currentIndex-1);
    }

    QImage GifManager::getNext()
    {
        return getCurrent(currentIndex+1);
    }

    void GifManager::setImage(const QImage &img, int index)
    {
        if(indexValid(index))
        {
            GifFrame frame=gifList.at(index);
            frame.image=img;
            gifList[index]=frame;
        }
    }

    bool GifManager::indexValid(int index) const
    {
        return (index>=0&&index<gifList.count());
    }

