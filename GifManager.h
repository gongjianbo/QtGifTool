#ifndef GIFMANAGER_H
#define GIFMANAGER_H

#include <QObject>
#include <QImage>
#include <QFutureWatcher>

struct GifFrame
{
    int index;
    QImage image;
    //目前用这个gif库deplay参数不正常，先忽略
    //int delay;
};

/**
 * @brief 管理GIF的导入导出等
 * @details
 * 使用QImageReader解析gif，使用gif.h生成gif
 * @note
 * 没看到Qt有生成gif的功能，参见QImageWriter支持类型
 * 而gif.h又只能写，且仅支持RGBA8作为输入格式
 * QImage是隐式共享的
 */
class GifManager : public QObject
{
    Q_OBJECT
public:
    explicit GifManager(QObject *parent = nullptr);
    ~GifManager();

    //加载gif
    void loadGif(const QString &path);
    //保存gif
    void saveGif(const QString &path);

    //图片数
    int getFrameCount() const;
    //当前图片
    void setCurrentIndex(int index);
    int getCurrentIndex() const;
    //获取图片帧
    //除了getImage，其他的可能会改变currentIndex值
    QImage getImage(int index=-1) const;
    QImage getCurrent(int index=-1);
    QImage getPrev();
    QImage getNext();
    //修改图片帧
    void setImage(const QImage &img,int index);

private:
    bool indexValid(int index) const;

signals:
    void gifChanged();
    void currentIndexChanged(int index);

private:
    int currentIndex=0;
    QFutureWatcher<QList<GifFrame>> readWatcher; //异步读gif
    QFutureWatcher<void> writeWatcher; //异步写gif
    QList<GifFrame> gifList; //图片列表
};

#endif // GIFMANAGER_H
