#include "include/mainwindow.h"
#include "QApplication"
#include <QDesktopWidget>
#include "include/Reader.h"
#include "QDebug"
#include "vector"

#include "include/RandomDecisionForest.h"


int main(int argc, char *argv[]) {

    QApplication app(argc, argv);
    MainWindow w;

    RandomDecisionForest* rdf = new RandomDecisionForest(10,15);
    rdf->readTrainingImageFiles();
    qDebug() << " CloudSize = " << rdf->pixelCloudSize();
    rdf->train();

    vector<Pixel*> res;
    QString filename = "/home/mahiratmis/Desktop/AnnotationResults/a/a_0.jpg";
    ImageInfo* img_inf = new ImageInfo('a',0);
    rdf->imageToPixels(res,filename,img_inf);
    if(rdf->test(res,'a'))
        qDebug() << "YEAYYYYY";
    else
        qDebug() << "COME ONNN";

    vector<Pixel*> res2;
    filename = "/home/mahiratmis/Desktop/AnnotationResults/b/b_10.jpg";
    img_inf = new ImageInfo('b',10);
    rdf->imageToPixels(res2,filename,img_inf);
    if(rdf->test(res,'b'))
        qDebug() << "YEAYYYYY";
    else
        qDebug() << "COME ONNN";


    QDesktopWidget dw;
    int x=dw.width()*0.3;
    int y=dw.height()*0.9;
    w.setFixedSize(x,y);

    w.show();
    return app.exec();
}
