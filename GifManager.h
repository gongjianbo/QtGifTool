#ifndef GIFMANAGER_H
#define GIFMANAGER_H

#include <QObject>
//#include <QMovie>
#include <QImageReader>
#include "

/**
 * @brief 管理GIF的导入导出等
 * @details 使用QImageReader解析gif，使用gif.h生成gif
 * @note 没看到Qt有生成gif的功能，参见QImageWriter支持类型
 */
class GifManager : public QObject
{
    Q_OBJECT
public:
    explicit GifManager(QObject *parent = nullptr);

    bool loadGif(const QString &path);
    int imageCount() const;

private:
    QImageReader gifReader;
};

#endif // GIFMANAGER_H
