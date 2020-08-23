#include "GifManager.h"

#include <QDebug>

GifManager::GifManager(QObject *parent) : QObject(parent)
{
    qDebug()<<"QImageReader surpports"<<gifReader.supportedImageFormats()<<gifReader.nextImageDelay();
}

bool GifManager::loadGif(const QString &path)
{
    gifReader.setFileName(path);
    gifReader.setFormat("gif");
    qDebug()<<gifReader.imageCount()<<gifReader.nextImageDelay();
    if(gifReader.imageCount()>0)
        return true;
    return false;
}

int GifManager::imageCount() const
{
    return gifReader.imageCount();
}
