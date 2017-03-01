#include "precompiled.h"

#include "rdf/RDFParams.h"
#include "rdf/RandomDecisionForestDialogGui.h"
#include "ui_RandomDecisionForestDialogGui.h"



void RandomDecisionForestDialogGui::initParamValues()
{
    ui->spinBox_NTrees->setValue(PARAMS.nTrees);
    ui->spinBox_MaxDepth->setValue(PARAMS.maxDepth);
    ui->spinBox_probX->setValue(PARAMS.probDistX);
    ui->spinBox_probY->setValue(PARAMS.probDistY);
    ui->spinBox_PixelsPerImage->setValue(PARAMS.pixelsPerImage);
    ui->spinBox_MinLeafPixels->setValue(PARAMS.minLeafPixels);
    ui->spinBox_MaxIteration->setValue(PARAMS.maxIteration);
    ui->spinBox_LabelCount->setValue(PARAMS.labelCount);
    ui->spinBox_TauRange->setValue(PARAMS.tauRange);
}

RandomDecisionForestDialogGui::RandomDecisionForestDialogGui(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RandomDecisionForestDialogGui)
{
    ui->setupUi(this);
    readSettings();
    initParamValues();
    m_nTreesForDetection = ui->spinBox_NTestTrees->value();
    QObject::connect(&SignalSenderInterface::instance(), SIGNAL(printsend(QString)), this,
                     SLOT(printMsgToTrainScreen(QString)));
    m_dataReaderGUI = new ReaderGUI();
    m_displayImagesGUI = new DisplayGUI();
    m_preprocessGUI = new PreProcessGUI();
    m_splitterVert = new QSplitter(this);
    m_splitterHori = new QSplitter(this);

    //Configure layout :
    m_splitterHori->addWidget(ui->LeftContainerWidget);
    m_splitterHori->addWidget(m_displayImagesGUI);

    m_splitterVert->addWidget(ui->tabWidget);
    m_splitterVert->addWidget(m_dataReaderGUI);
    m_splitterVert->addWidget(m_preprocessGUI);
    m_splitterVert->addWidget(ui->console);
    m_splitterVert->setOrientation(Qt::Vertical);
    QObject::connect(m_dataReaderGUI, SIGNAL(imagesLoaded(bool)), this, SLOT(onImagesLoaded(bool)));
    ui->verticalLayout_LeftContainer->addWidget(m_splitterVert);
    ui->gridLayout_RDFGUI->addWidget(m_splitterHori, 1,0);
}

RandomDecisionForestDialogGui::~RandomDecisionForestDialogGui()
{
    delete ui;
}

void RandomDecisionForestDialogGui::onImagesLoaded(bool)
{
    m_forest.setDataSet(*m_dataReaderGUI->DS());
    m_displayImagesGUI->setImageSet(m_dataReaderGUI->DS()->m_ImagesVector);

}

void RandomDecisionForestDialogGui::onLabelsLoaded()
{
    PARAMS.testDir = QFileDialog::getExistingDirectory(this, tr("Open Image Directory"), PARAMS.trainImagesDir,
                                                       QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->console->append("Test images read");
    ui->console->setText(PARAMS.testDir);
}

void RandomDecisionForestDialogGui::onTrain()
{
    m_forest.setParams(PARAMS);
    m_forest.setDataSet(*m_dataReaderGUI->DS());
    if (m_forest.trainForest())
        ui->console->append("Forest Trained ! ");
    else
        ui->console->append("Failed to Train Forest! (Forest DS is not set) ");
    ui->console->repaint();
}

void RandomDecisionForestDialogGui::onTest()
{
    if (!m_isTestDataProcessed)
        m_isTestDataProcessed = true;
    // TODO: fix preprocessing
    DataSet *DS = m_dataReaderGUI->DS();
    int totalImgs = DS->m_ImagesVector.size();
    m_forest.setNTreesForDetection(m_nTreesForDetection);
    if (totalImgs == 0)
    {
        QMessageBox *msgBox = new QMessageBox();
        msgBox->setWindowTitle("Error");
        msgBox->setText("You Should Load Test Data First!");
        msgBox->show();
        return;
    }
    float accuracy = 0;
    for (int i = 0; i < totalImgs; ++i)
    {
        cv::Mat img = DS->m_ImagesVector[i].clone();
        int label{};
        float conf{};
        m_forest.detect(img, label, conf);
        if (label == DS->m_labels[i])
            ++accuracy;
    }
    accuracy /= totalImgs;
    printMsgToTestScreen(QString::number(m_nTreesForDetection) + " tree accuracy: " + QString::number(100 * accuracy));
}

void RandomDecisionForestDialogGui::onPreProcess()
{
    m_forest.preprocessDS();
    m_displayImagesGUI->setImageSet(m_dataReaderGUI->DS()->m_ImagesVector);
    m_forest.DS().m_isProcessed = true;
}

void RandomDecisionForestDialogGui::applySobel(std::vector<cv::Mat> &images)
{
    int scale = 1;
    int delta = 0;
    int ddepth = CV_16S;
    /// Generate grad_x and grad_y
    cv::Mat grad_x, grad_y;
    cv::Mat abs_grad_x, abs_grad_y;
    for (auto &img : images)
    {
        cv::GaussianBlur(img, img, cv::Size(3, 3), 0);
        //        cv::Canny( img, img, lowThreshold, lowThreshold*ratio, kernel_size );
        /// Gradient X
        //Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
        cv::Sobel(img, grad_x, ddepth, 1, 0, 3, scale, delta, cv::BORDER_DEFAULT);
        cv::convertScaleAbs(grad_x, abs_grad_x);
        /// Gradient Y
        //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
        cv::Sobel(img, grad_y, ddepth, 0, 1, 3, scale, delta, cv::BORDER_DEFAULT);
        cv::convertScaleAbs(grad_y, abs_grad_y);
        /// Total Gradient (approximate)
        cv::addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, img);
        //        cv::imshow( "window_name", img);
        //        cv::waitKey();
    }
}

void RandomDecisionForestDialogGui::applyCanny(std::vector<cv::Mat> &images)
{
    int lowThreshold = 50;
    int ratio = 3;
    int kernel_size = 3;
    int count = 0;
    for (auto &img : images)
    {
        //        cv::imshow( "window_fray", img);
        cv::GaussianBlur(img, img, cv::Size(11, 11), 0);
        cv::Canny(img, img, lowThreshold, lowThreshold * ratio, kernel_size);
        //        std::cout << m_trainDataReaderGUI->DS()->m_labels[count] << std::endl;
        //        cv::imshow( "window_name", img);
        //        cv::waitKey();
    }
}

void RandomDecisionForestDialogGui::image_at_classified_as(int index, char label)
{
    ui->console->append("Image " + QString::number(
                                      index) + " is classified as " + label);
    ui->console->repaint();
}

void RandomDecisionForestDialogGui::resultPercetange(double accuracy)
{
    ui->console->append(" Classification Accuracy : " + QString::number(
                                      accuracy) + "%");
    ui->console->repaint();
}

void RandomDecisionForestDialogGui::printMsgToTrainScreen(QString msg)
{
    ui->console->append(msg);
    ui->console->repaint();
}

void RandomDecisionForestDialogGui::printMsgToTestScreen(QString msg)
{
    ui->console->append(msg);
    ui->console->repaint();
}

void RandomDecisionForestDialogGui::onLoad()
{
    QString selfilter = tr("BINARY (*.bin *.txt)");
    QString fname = QFileDialog::getOpenFileName(
                        this,
                        tr("Load Forest Directory"),
                        QDir::currentPath(),
                        tr("BINARY (*.bin);;TEXT (*.txt);;All files (*.*)"),
                        &selfilter
                    );
    m_forest.loadForest(fname);
    //    m_forest->printForest();
    qDebug() << "LOAD FOREST PRINTED" ;
}

void RandomDecisionForestDialogGui::onSave()
{
    QString dirname = QFileDialog::getExistingDirectory(this,
                                                        tr("Open Save Directory"), PARAMS.trainImagesDir,
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    QString fname = "nT_" + QString::number(m_forest.params().nTrees) + "_D_" +
                    QString::number(m_forest.params().maxDepth)
                    //                    + "_nTImg_" + QString::number(m_forest.m_imagesContainer.size()) // TODO: add later
                    + "_nPxPI_" + QString::number(m_forest.params().pixelsPerImage)
                    + ".bin";
    m_forest.saveForest(dirname + "/" + fname);
    qDebug() << "SAVED FOREST PRINTED" ;
}



void RandomDecisionForestDialogGui::closeEvent(QCloseEvent *event)
{
    writeSettings();
    m_dataReaderGUI->writeSettings();
    event->accept();
}

void RandomDecisionForestDialogGui::writeSettings()
{
    QSettings settings("VVGLab", "RDFDialogGUI");
    settings.beginGroup("PARAMS");
    settings.setValue("nTrees", PARAMS.nTrees);
    settings.setValue("maxDepth", PARAMS.maxDepth);
    settings.setValue("probDistX", PARAMS.probDistX);
    settings.setValue("probDistY", PARAMS.probDistY);
    settings.setValue("pixelsPerImage", PARAMS.pixelsPerImage);
    settings.setValue("minLeafPixels", PARAMS.minLeafPixels);
    settings.setValue("maxIteration", PARAMS.maxIteration);
    settings.setValue("labelCount", PARAMS.labelCount);
    settings.setValue("tauRange", PARAMS.tauRange);
    settings.endGroup();
}

void RandomDecisionForestDialogGui::readSettings()
{
    QSettings settings("VVGLab", "RDFDialogGUI");
    settings.beginGroup("PARAMS");
    PARAMS.nTrees = settings.value("nTrees", 12).toInt();
    PARAMS.maxDepth = settings.value("maxDepth", 20).toInt();
    PARAMS.probDistX = settings.value("probDistX", 30).toInt();
    PARAMS.probDistY = settings.value("probDistY", 30).toInt();
    PARAMS.pixelsPerImage = settings.value("pixelsPerImage", 200).toInt();
    PARAMS.minLeafPixels = settings.value("minLeafPixels", 5).toInt();
    PARAMS.maxIteration = settings.value("maxIteration", 100).toInt();
    PARAMS.labelCount = settings.value("labelCount", 26).toInt();
    PARAMS.tauRange = settings.value("tauRange", 127).toInt();
    settings.endGroup();
}

void RandomDecisionForestDialogGui::onNTreesChanged(int value)
{
    PARAMS.nTrees = value;
    ui->spinBox_NTestTrees->setMaximum(value);
}

void RandomDecisionForestDialogGui::onNTestTreesChanged(int value)
{
    m_nTreesForDetection = value;
}

void RandomDecisionForestDialogGui::onMaxDepthChanged(int value)
{
    PARAMS.maxDepth = value;
}

void RandomDecisionForestDialogGui::onProbDistXChanged(int value)
{
    PARAMS.probDistX = value;
}

void RandomDecisionForestDialogGui::onProbDistYChanged(int value)
{
    PARAMS.probDistY = value;
}

void RandomDecisionForestDialogGui::onPixelsPerImageChanged(int value)
{
    PARAMS.pixelsPerImage = value;
}

void RandomDecisionForestDialogGui::onMinLeafPixelsChanged(int value)
{
    PARAMS.minLeafPixels = value;
}

void RandomDecisionForestDialogGui::onMaxIterationChanged(int value)
{
    PARAMS.maxIteration = value;
}

void RandomDecisionForestDialogGui::onLabelCountChanged(int value)
{
    PARAMS.labelCount = value;
}

void RandomDecisionForestDialogGui::onTauRangeChanged(int value)
{
    PARAMS.tauRange = value;
}
