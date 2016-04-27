#include "RandomDesicionTree.h"
#include "include/RandomDecisionForest.h"

// create the tree
void RandomDecisionTree::train()
{
    subSample();
    Node *root = new Node(1, false);
    root->m_tau = generateTau();
    generateTeta(root->m_teta1, m_probe_distanceX, m_probe_distanceY);
    generateTeta(root->m_teta2, m_probe_distanceX, m_probe_distanceY);
    m_nodes[0] = root;
    m_depth=1;
    m_numOfLeaves = 0;
    constructTree(*root, m_pixelCloud);
    emit  m_DF->treeConstructed();
}

// create child nodes from nodes by tuning each node
void RandomDecisionTree::constructTree( Node& cur_node, PixelCloud& pixels)
{
    //float rootEntropy = calculateEntropyOfVector(pixels);
    int current_depth = log2(cur_node.m_id) + 1;
    if(current_depth > m_depth)
        m_depth = current_depth;
    //no more child construction
    // min etropy removed
    if( current_depth >= m_maxDepth || pixels.size() <= m_minLeafPixelCount )
    {
        m_nodes[cur_node.m_id-1]->m_isLeaf = true;
        m_nodes[cur_node.m_id-1]->m_hist = createHistogram(pixels, m_DF->m_params.labelCount);
        ++m_numOfLeaves;
        return;
    }

    tuneParameters(pixels, cur_node);
    std::vector<Pixel*> left;
    std::vector<Pixel*> right;
    divide(m_DF->m_DS, pixels, left, right, cur_node);


    Node *leftChildNode = new Node(2*cur_node.m_id,false);
    leftChildNode->m_tau = generateTau();
    generateTeta(leftChildNode->m_teta1, m_probe_distanceX, m_probe_distanceY);
    generateTeta(leftChildNode->m_teta2, m_probe_distanceX, m_probe_distanceY);
    m_nodes[leftChildNode->m_id-1] = leftChildNode;
    constructTree(*leftChildNode,left);

    Node *rightChildNode = new Node(2*cur_node.m_id+1, false);
    rightChildNode->m_tau = generateTau();
    generateTeta(rightChildNode->m_teta1, m_probe_distanceX, m_probe_distanceY);
    generateTeta(rightChildNode->m_teta2, m_probe_distanceX, m_probe_distanceY);
    m_nodes[rightChildNode->m_id-1] = rightChildNode;
    constructTree(*rightChildNode,right);

}

void RandomDecisionTree::subSample()
{
    int sampleId=0;
    for(auto &image : m_DF->m_DS.m_trainImagesVector)
    {
        auto label = m_DF->m_DS.m_trainlabels[sampleId];
        ImageInfo *img_inf = new ImageInfo(label, ++sampleId);

        // draw perImagePixel pixels from image
        // bootstrap, subsample
        for(int k=0; k<m_DF->m_params.pixelsPerImage; ++k)
        {
            int i = (rand() % (image.rows-2*m_probe_distanceY)) + m_probe_distanceY;
            int j = (rand() % (image.cols-2*m_probe_distanceX)) + m_probe_distanceX;
            auto intensity = image.at<uchar>(i,j);
            auto *px = new Pixel(Coord(i,j),intensity,img_inf);
            m_pixelCloud.push_back(px);
        }
    }
}

// find best teta and taw parameters for the given node
void RandomDecisionTree::tuneParameters(PixelCloud& parentPixels, Node& parent)
{
    std::vector<Pixel*> left;
    std::vector<Pixel*> right;
    int maxTau;
    Coord maxTeta1,maxTeta2;
    float maxGain=0;
    int itr=0;

    auto nLabels = m_DF->m_params.labelCount;
    while(itr < m_DF->m_params.maxIteration)
    {
        //printNode(parent);
        divide(m_DF->m_DS, parentPixels, left, right, parent);
        //qDebug() << "Left " << left.size() << "Right " << right.size();
        float leftChildEntr = calculateEntropyOfVector(left, nLabels);
        //qDebug() << "EntLeft" << leftChildEntr ;
        float rightChildEntr = calculateEntropyOfVector(right, nLabels);
        //qDebug() << "EntRight" << rightChildEntr ;
        int sizeLeft  = left.size();
        int sizeRight = right.size();
        float totalsize = parentPixels.size() ;
        float avgEntropyChild  = (sizeLeft/totalsize) * leftChildEntr;
              avgEntropyChild += (sizeRight/totalsize)* rightChildEntr;

        float parentEntr = calculateEntropyOfVector(parentPixels, nLabels);
        float infoGain = parentEntr - avgEntropyChild;
        // qDebug() << "InfoGain" << infoGain ;

        // Non-improving epoch :
        if(infoGain > maxGain)
        {
            maxTeta1 = parent.m_teta1;
            maxTeta2 = parent.m_teta2;
            maxTau   = parent.m_tau;
            maxGain = infoGain;
            itr = 0 ;
        }
        else
        {
            ++itr;
        }

        left.clear();
        right.clear();
        generateTeta(parent.m_teta1, m_probe_distanceX, m_probe_distanceY);
        generateTeta(parent.m_teta2, m_probe_distanceX, m_probe_distanceY);
        parent.m_tau = generateTau();
    }

    parent.m_tau = maxTau;
    parent.m_teta1 = maxTeta1;
    parent.m_teta2 = maxTeta2;
}


bool RandomDecisionTree::isPixelSizeConsistent()
{
    auto nPixelsOnLeaves=0u;
    for(Node *node : m_nodes)
        if(node->m_isLeaf)
            nPixelsOnLeaves += getTotalNumberOfPixels(node->m_hist);

    return m_pixelCloud.size() == nPixelsOnLeaves;
}

void RandomDecisionTree::toString()
{
    qDebug() << "Size  : " << m_nodes.size() ;
    qDebug() << "Depth : " << m_depth;
    qDebug() << "Leaves: " << m_numOfLeaves;
    int count = 0 ;
    for (auto *node : m_nodes)
    {
        if(node->m_isLeaf)
        {
            ++count;
            printHistogram(node->m_hist);
        }
    }
    qDebug()<< "Number of leaves : " << count ;
}

void RandomDecisionTree::printPixelCloud()
{
    for(auto pPixel : m_pixelCloud)
        printPixel(pPixel);
}

void RandomDecisionTree::printPixel(Pixel* px)
{
    qDebug() << "Pixel{ Coor("    << px->position.m_dy     << ","     << px->position.m_dx
             << ") Label("        << px->imgInfo->label << ") Id(" << px->imgInfo->sampleId
             << ") = "            << px->intensity << "}";
}

void RandomDecisionTree::printNode(Node& node){
    QString res = "NODE {";
    qDebug() << "Id:"   << node.m_id
             << "Taw: " << node.m_tau
             << "Q1{" << node.m_teta1.m_dy << "," << node.m_teta1.m_dx
             << "Q2{" << node.m_teta2.m_dy << "," << node.m_teta2.m_dx;
    printHistogram(node.m_hist);
    qDebug() << "}";
}