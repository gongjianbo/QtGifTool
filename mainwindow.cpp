#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QtConcurrentRun>
#include <QDebug>
#include "gif_lib.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<GifInfo>("GifInfo");

    //测试接口是否可以调用
    qDebug()<<"Version:"<<GIFLIB_MAJOR<<GIFLIB_MINOR<<GIFLIB_RELEASE;
    qDebug()<<"GifErrorString:"<<GifErrorString(D_GIF_ERR_NOT_GIF_FILE);

    //翻页
    connect(ui->btnPrev,&QPushButton::clicked,this,&MainWindow::displayPrev);
    connect(ui->btnNext,&QPushButton::clicked,this,&MainWindow::displayNext);
    //点击读取Gif
    connect(ui->btnDecGif,&QPushButton::clicked,this,&MainWindow::decodeGif);
    connect(ui->btnDecGifStream,&QPushButton::clicked,this,&MainWindow::decodeGifStream);
    //点击选择图片列表
    connect(ui->btnSelectImg,&QPushButton::clicked,this,&MainWindow::selectImages);
    //点击生成Gif
    connect(ui->btnEncGif,&QPushButton::clicked,this,&MainWindow::encodeGif);

    //线程执行完
    connect(&taskWatcher,&QFutureWatcher<void>::finished,this,[this]{
        ui->centralwidget->setEnabled(true);
        qDebug()<<"task finished.";
    });

    //解析完gif
    connect(this,&MainWindow::decodeGifFinished,this,[this](const GifInfo &info){
        if(info.images.isEmpty())
            return;
        imgList=info.images;
        ui->imageLabel->setPixmap(QPixmap::fromImage(imgList.first()));
        ui->spinPageCount->setValue(imgList.count());
        ui->spinPageCurrent->setValue(1);
        ui->spinWidth->setValue(info.width);
        ui->spinHeight->setValue(info.height);
        ui->spinInterval->setValue(info.interval);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    if(!taskWatcher.isFinished())
        taskWatcher.waitForFinished();
}

void MainWindow::displayPrev()
{
    const int prev_index=ui->spinPageCurrent->value()-2;
    if(prev_index>=0&&prev_index<imgList.count()){
        ui->spinPageCurrent->setValue(prev_index+1);
        ui->imageLabel->setPixmap(QPixmap::fromImage(imgList.at(prev_index)));
    }
}

void MainWindow::displayNext()
{
    const int next_index=ui->spinPageCurrent->value();
    if(next_index>0&&next_index<imgList.count()){
        ui->spinPageCurrent->setValue(next_index+1);
        ui->imageLabel->setPixmap(QPixmap::fromImage(imgList.at(next_index)));
    }
}

void MainWindow::decodeGif()
{
    if(!taskWatcher.isFinished())
        return;

    const QString gif_path=QFileDialog::getOpenFileName(
                this, "Open Gif","","Gif (*.gif)");
    if(gif_path.isEmpty())
        return;

    ui->centralwidget->setEnabled(false);
    QFuture<void> future=QtConcurrent::run([=]{
        qDebug()<<"gif to image:"<<gif_path;
        const QByteArray path_temp = gif_path.toLocal8Bit();
        const char *path_cstr = path_temp.constData();
        int error_code;
        //解码器打开文件
        GifFileType *gif_file = DGifOpenFileName(path_cstr, &error_code);
        if (gif_file  == NULL) {
            const char* gif_error = GifErrorString(error_code);
            qDebug() << "open error:" << gif_error;
            return;
        }
        //DGifSlurp相对于流式接口更方便点，一次解析所有
        error_code = DGifSlurp(gif_file);
        if(error_code!=GIF_OK){
            qDebug()<<"gif slurp error.";
            return;
        }

        //保存解析结果
        GifInfo gif_info;
        gif_info.width=gif_file->SWidth;
        gif_info.height=gif_file->SHeight;
        gif_info.colors=(1 << gif_file->SColorResolution);
        //qDebug()<<"screen size(w*h)"<<gif_file->SWidth<<"*"<<gif_file->SHeight;
        //qDebug()<<"screen colors"<<(1 << gif_file->SColorResolution)
        //       <<"screen bgcolor"<<gif_file->SBackGroundColor
        //      <<"pixel aspect byte"<<(unsigned)gif_file->AspectByte;

        //存color index
        QVector<GifByteType> screen_buf(gif_file->SWidth*gif_file->SHeight);
        memset(screen_buf.data(),gif_file->SBackGroundColor,sizeof(GifByteType)*screen_buf.size());
        //暂存上一帧图像
        QImage img_temp=QImage(gif_file->SWidth,gif_file->SHeight,QImage::Format_RGB32);
        //TransparentColor表示保留上一帧状态的color index
        int trans_color = -1;
        //逐帧转换为QImage
        for (int im = 0; im < gif_file->ImageCount; im++)
        {
            SavedImage *image = &gif_file->SavedImages[im];
            //获取间隔、TransparentColor等信息
            for (ExtensionBlock *ep = image->ExtensionBlocks;
                 ep < image->ExtensionBlocks + image->ExtensionBlockCount;
                 ep++)
            {
                bool last = (ep - image->ExtensionBlocks == (image->ExtensionBlockCount - 1));
                if (ep->Function == GRAPHICS_EXT_FUNC_CODE)
                {
                    GraphicsControlBlock gcb;
                    if (DGifExtensionToGCB(ep->ByteCount, ep->Bytes, &gcb) == GIF_ERROR) {
                        qDebug()<<"invalid graphics control block";
                        return;
                    }
                    //qDebug()<<"disposal mode"<< gcb.DisposalMode;
                    //qDebug()<<"user input fla"<<(gcb.UserInputFlag ? "on" : "off");
                    //qDebug()<<"delay"<<gcb.DelayTime;
                    //qDebug()<<"transparent index"<<gcb.TransparentColor;
                    trans_color=gcb.TransparentColor;
                    //DelayTime的单位为10 ms
                    gif_info.interval=gcb.DelayTime*10;
                }
                else if (!last
                         && ep->Function == APPLICATION_EXT_FUNC_CODE
                         && ep->ByteCount >= 11
                         && (ep+1)->ByteCount >= 3
                         && memcmp(ep->Bytes, "NETSCAPE2.0", 11) == 0)
                {
                    unsigned char *params = (++ep)->Bytes;
                    unsigned int loopcount = params[1] | (params[2] << 8);
                    //qDebug()<< "netscape loop "<<loopcount;
                    //第一帧一般包含循环次数的ExtensionBlock， 0则循环包房
                    gif_info.loopcount=loopcount;
                }
                else
                {
                    //其他信息暂不处理
                    while (!last && ep[1].Function == CONTINUE_EXT_FUNC_CODE) {
                        ++ep;
                        last = (ep - image->ExtensionBlocks == (image->ExtensionBlockCount - 1));
                    }
                }
            }

            //image->ImageDesc指定了这帧图的刷新区域，类似于opencv的roi
            //qDebug()<<"left"<<image->ImageDesc.Left
            //       <<"top"<<image->ImageDesc.Top
            //      <<"width"<<image->ImageDesc.Width
            //     <<"height"<<image->ImageDesc.Height
            //    <<"color"<<image->ImageDesc.ColorMap->ColorCount;
            if (image->ImageDesc.Interlace)
                qDebug()<<"image interlaced";

            //如果没有局部色彩管理就用全局的
            ColorMapObject *color_map = (image->ImageDesc.ColorMap ? image->ImageDesc.ColorMap : gif_file->SColorMap);
            if(color_map == NULL){
                qDebug()<<"color map error.";
                return;
            }

            //保留上一帧的图像主要是因为当前可能只刷新部分
            QImage img=img_temp.copy();
            GifColorType *color_map_entry;
            //遍历行列，把每个像素点的颜色填到img
            for (int i = image->ImageDesc.Top; i < image->ImageDesc.Top+image->ImageDesc.Height; i++)
            {
                memcpy(screen_buf.data()+i*gif_file->SWidth+image->ImageDesc.Left,
                       image->RasterBits+(i-image->ImageDesc.Top)*image->ImageDesc.Width,
                       image->ImageDesc.Width);
                for (int j = image->ImageDesc.Left; j < image->ImageDesc.Left+image->ImageDesc.Width; j++)
                {
                    if(trans_color!=-1&&trans_color==screen_buf[i*gif_file->SWidth+j])
                        continue;
                    color_map_entry = &color_map->Colors[screen_buf[i*gif_file->SWidth+j]];
                    img.setPixelColor(j,i,QColor(
                                          color_map_entry->Red,
                                          color_map_entry->Green,
                                          color_map_entry->Blue));
                }
            }
            img_temp=img;
            gif_info.images.push_back(img);
        }

        //关闭文件
        if (DGifCloseFile(gif_file, &error_code) == GIF_ERROR){
            const char* gif_error = GifErrorString(error_code);
            qDebug() << "close error:" << gif_error;
            return;
        }
        qDebug()<<"gif to image finish";
        emit decodeGifFinished(gif_info);
    });
    taskWatcher.setFuture(future);
}

void MainWindow::decodeGifStream()
{
    if(!taskWatcher.isFinished())
        return;

    const QString gif_path=QFileDialog::getOpenFileName(
                this, "Open Gif","","Gif (*.gif)");
    if(gif_path.isEmpty())
        return;

    ui->centralwidget->setEnabled(false);
    QFuture<void> future=QtConcurrent::run([=]{
        qDebug()<<"gif to image:"<<gif_path;
        const QByteArray path_temp = gif_path.toLocal8Bit();
        const char *path_cstr = path_temp.constData();
        int error_code;
        //解码器打开文件
        GifFileType *gif_file = DGifOpenFileName(path_cstr, &error_code);
        if (gif_file  == NULL) {
            const char* gif_error = GifErrorString(error_code);
            qDebug() << "open error:" << gif_error;
            return;
        }
        //qDebug() << "gif height:" << gif_file->SHeight << " width:" << gif_file->SWidth;
        if (gif_file->SHeight<0 || gif_file->SWidth<0) {
            qDebug() << "size error.";
            return;
        }
        //保存解析结果
        GifInfo gif_info;
        gif_info.width=gif_file->SWidth;
        gif_info.height=gif_file->SHeight;
        gif_info.colors=(1 << gif_file->SColorResolution);

        QVector<GifByteType> screen_buf(gif_file->SWidth*gif_file->SHeight);
        //The way Interlaced image should be read - offsets and jumps...
        int interlaced_offset[] = { 0, 4, 2, 1 };
        int interlaced_jumps[] = { 8, 8, 4, 2 };
        GifByteType *extension;
        GifRecordType record_type;
        ColorMapObject *color_map;
        int ext_code;
        int the_col;
        int the_row;
        int the_width;
        int the_height;
        int trans_color = -1;
        memset(screen_buf.data(),gif_file->SBackGroundColor,sizeof(GifByteType)*screen_buf.size());
        QImage img_temp=QImage(gif_file->SWidth,gif_file->SHeight,QImage::Format_RGB32);
        do {
            error_code = DGifGetRecordType(gif_file, &record_type);
            if (error_code == GIF_ERROR){
                const char* gif_error = GifErrorString(gif_file->Error);
                qDebug() << "get record error:" << gif_error;
                return;
            }
            //qDebug()<<"type"<<record_type;
            switch (record_type)
            {
            case IMAGE_DESC_RECORD_TYPE:
                error_code = DGifGetImageDesc(gif_file);
                if (error_code == GIF_ERROR) {
                    const char* gif_error = GifErrorString(gif_file->Error);
                    qDebug() << "get image error:" << gif_error;
                    return;
                }
                the_row = gif_file->Image.Top;
                the_col = gif_file->Image.Left;
                the_width = gif_file->Image.Width;
                the_height = gif_file->Image.Height;
                //qDebug()<<the_row<<the_col<<the_width<<the_height<<gif_file->Image.Interlace<<gif_file->SBackGroundColor;
                if (the_col+the_width>gif_file->SWidth || the_row+the_height>gif_file->SHeight) {
                    qDebug()<<"Image X is not confined to screen dimension, aborted.";
                    return;
                }
                if (gif_file->Image.Interlace){
                    for (int i = 0; i < 4; i++)
                    {
                        for (int j=the_row+interlaced_offset[i]; j<the_row+the_height; j+=interlaced_jumps[i])
                        {
                            error_code = DGifGetLine(gif_file, &screen_buf[j*4+the_col], the_width);
                            if (error_code == GIF_ERROR){
                                const char* gif_error = GifErrorString(gif_file->Error);
                                qDebug() << "get line error:" << gif_error;
                            }
                        }
                    }
                }else{
                    for (int i = 0; i < the_height; i++)
                    {
                        error_code = DGifGetLine(gif_file, &screen_buf[(the_row+i)*gif_file->SWidth+the_col],the_width);
                        if (error_code == GIF_ERROR){
                            const char* gif_error = GifErrorString(gif_file->Error);
                            qDebug() << "get line error:" << gif_error;
                        }
                    }
                }
                //如果没有局部色彩管理就用全局的
                color_map = (gif_file->Image.ColorMap ? gif_file->Image.ColorMap : gif_file->SColorMap);
                if(color_map == NULL){
                    qDebug()<<"color map error.";
                    return;
                }else{
                    QImage img=img_temp.copy();
                    GifColorType *color_map_entry;
                    for (int i = the_row; i < the_height+the_row; i++)
                    {
                        for (int j = the_col; j < the_width+the_col; j++)
                        {
                            if(trans_color!=-1&&trans_color==screen_buf[i*gif_file->SWidth+j])
                                continue;
                            color_map_entry = &color_map->Colors[screen_buf[i*gif_file->SWidth+j]];
                            img.setPixelColor(j,i,QColor(
                                                  color_map_entry->Red,
                                                  color_map_entry->Green,
                                                  color_map_entry->Blue));
                        }
                    }
                    img_temp=img;
                    gif_info.images.push_back(img);
                }
                break;
            case EXTENSION_RECORD_TYPE:
                error_code = DGifGetExtension(gif_file, &ext_code, &extension);
                if (error_code == GIF_ERROR){
                    const char* gif_error = GifErrorString(gif_file->Error);
                    qDebug() << "get extension error:" << gif_error;
                    return;
                }
                //qDebug()<<"extension"<<ext_code;
                if(ext_code==GRAPHICS_EXT_FUNC_CODE){
                    GraphicsControlBlock gcb;
                    if(extension==NULL){
                        qDebug()<<"gcb extension null";
                        return;
                    }
                    error_code = DGifExtensionToGCB(extension[0], extension+1, &gcb);
                    if (error_code == GIF_ERROR) {
                        const char* gif_error = GifErrorString(gif_file->Error);
                        qDebug() << "extension to gcb error:" << gif_error;
                        return;
                    }
                    trans_color = gcb.TransparentColor;
                    //qDebug()<<"gcb:"<<gcb.DelayTime<<gcb.TransparentColor;
                    gif_info.interval=gcb.DelayTime*10;
                }
                while (extension != NULL)
                {
                    error_code = DGifGetExtensionNext(gif_file, &extension);
                    if (error_code == GIF_ERROR){
                        const char* gif_error = GifErrorString(gif_file->Error);
                        qDebug() << "get extension next error:" << gif_error;
                    }
                }
                break;
            case TERMINATE_RECORD_TYPE:
                break;
            default:
                break;
            }
        } while (record_type != TERMINATE_RECORD_TYPE);

        //关闭文件
        if (DGifCloseFile(gif_file, &error_code) == GIF_ERROR){
            const char* gif_error = GifErrorString(error_code);
            qDebug() << "close error:" << gif_error;
            return;
        }
        qDebug()<<"gif to image finish";
        emit decodeGifFinished(gif_info);
    });
    taskWatcher.setFuture(future);
}

void MainWindow::selectImages()
{
    if(!taskWatcher.isFinished())
        return;
    ui->centralwidget->setEnabled(false);
    QFuture<void> future=QtConcurrent::run([=]{

    });
    taskWatcher.setFuture(future);
}

void MainWindow::encodeGif()
{
    QList<QImage> gif_list=imgList;
    int interval=ui->spinInterval->value();
    if(!taskWatcher.isFinished()||gif_list.isEmpty())
        return;
    const QString gif_path=QFileDialog::getSaveFileName(
                this, "Save Gif","","Gif (*.gif)");
    if(gif_path.isEmpty())
        return;

    ui->centralwidget->setEnabled(false);
    QFuture<void> future=QtConcurrent::run([=]{
        qDebug()<<"image to gif begin:"<<gif_path;
        const QByteArray path_temp = gif_path.toLocal8Bit();
        const char *path_cstr = path_temp.constData();

        QImage img_first(gif_list.first());
        const int gif_width=img_first.width();
        const int gif_height=img_first.height();
        //qDebug()<<"width"<<gif_width<<"height"<<gif_height;
        QVector<GifByteType> red_buffer(gif_width*gif_height);
        QVector<GifByteType> green_buffer(gif_width*gif_height);
        QVector<GifByteType> blue_buffer(gif_width*gif_height);

        //以编码方式打开文件
        int error_code;
        GifFileType *gif_file=EGifOpenFileName(path_cstr,false,&error_code);
        if(gif_file == NULL){
            const char* gif_error = GifErrorString(error_code);
            qDebug() << "open error:" << gif_error;
            return;
        }

        gif_file->SWidth=gif_width;
        gif_file->SHeight=gif_height;
        gif_file->SColorResolution=8;
        gif_file->SBackGroundColor=0;
        gif_file->SColorMap=NULL;
        gif_file->SavedImages=NULL;
        gif_file->ImageCount=0;
        gif_file->ExtensionBlockCount=0;
        gif_file->ExtensionBlocks=NULL;

        for(int i = 0; i < gif_list.count(); i++) {
            QImage img(gif_list.at(i));
            SavedImage *image = GifMakeSavedImage(gif_file, NULL);
            if(img.width()<gif_width||img.height()<gif_height)
                continue;
            for(int row=0;row<gif_height;row++)
            {
                for(int col=0;col<gif_width;col++)
                {
                    red_buffer[row*gif_width+col]=img.pixelColor(col,row).red();
                    green_buffer[row*gif_width+col]=img.pixelColor(col,row).green();
                    blue_buffer[row*gif_width+col]=img.pixelColor(col,row).blue();
                }
            }
            int map_size=(1<<gif_file->SColorResolution);
            if ((image->ImageDesc.ColorMap = GifMakeMapObject(map_size, NULL)) == NULL){
                qDebug()<<"Failed to allocate memory required, aborted.";
                return;
            }
            image->RasterBits  = (GifPixelType *)malloc(sizeof(GifPixelType) * gif_width * gif_height);
            //5.2.0 GifQuantizeBuffer函数已移除
            //但是自己去做rgb转256色太麻烦，所以我又加上了
            //map_size调用后会被修改为实际值
            if (GifQuantizeBuffer(gif_width, gif_height, &map_size,
                                  red_buffer.data(),
                                  green_buffer.data(),
                                  blue_buffer.data(),
                                  image->RasterBits,
                                  image->ImageDesc.ColorMap->Colors) == GIF_ERROR)
            {
                qDebug()<<"error";
                return;
            }

            //qDebug()<<"MakeSavedImage"<<i<<"mapsize"<<map_size;
            image->ImageDesc.Left = 0;
            image->ImageDesc.Top = 0;
            image->ImageDesc.Width = gif_width;
            image->ImageDesc.Height = gif_height;
            image->ImageDesc.Interlace = false;
            //image->ImageDesc.ColorMap = color_map;
            //image->RasterBits = out_buffer;

            GraphicsControlBlock gcb;
            gcb.DisposalMode = DISPOSAL_UNSPECIFIED;
            gcb.DelayTime = interval/10;
            gcb.UserInputFlag = false;
            gcb.TransparentColor = NO_TRANSPARENT_COLOR;

            //qDebug()<<"GCB To Saved"<<i;
            EGifGCBToSavedExtension(&gcb, gif_file, i);

            //把循环次数写到第一帧对应的扩展块
            if(i==0){
                unsigned char params[3] = {1, 0, 0};
                //Create a Netscape 2.0 loop block
                if (GifAddExtensionBlock(&image->ExtensionBlockCount,
                                         &image->ExtensionBlocks,
                                         APPLICATION_EXT_FUNC_CODE,
                                         11,
                                         (unsigned char *)"NETSCAPE2.0")==GIF_ERROR) {
                    qDebug()<<"out of memory while adding loop block.";
                    return;
                }
                //params[1] = (intval & 0xff);
                //params[2] = (intval >> 8) & 0xff;
                if (GifAddExtensionBlock(&image->ExtensionBlockCount,
                                         &image->ExtensionBlocks,
                                         0, sizeof(params), params) == GIF_ERROR) {
                    qDebug()<<"out of memory while adding loop continuation.";
                    return;
                }
            }
        }

        //qDebug()<<gif_file->ImageCount;
        int saved_count=gif_file->ImageCount;
        SavedImage *saved_images=gif_file->SavedImages;
        //关闭GIF并释放相关存储。
        EGifSpew(gif_file);
        qDebug()<<gif_file->ImageCount;
        {
            SavedImage *sp;
            for (sp = saved_images;
                 sp < saved_images + saved_count; sp++) {
                if (sp->ImageDesc.ColorMap != NULL) {
                    GifFreeMapObject(sp->ImageDesc.ColorMap);
                    sp->ImageDesc.ColorMap = NULL;
                }

                if (sp->RasterBits != NULL)
                    free((char *)sp->RasterBits);

                GifFreeExtensions(&sp->ExtensionBlockCount, &sp->ExtensionBlocks);
            }
            free((char *)saved_images);
            saved_images = NULL;
            sp=NULL;
        }

        qDebug()<<"image to gif finish.";
    });
    taskWatcher.setFuture(future);
}

