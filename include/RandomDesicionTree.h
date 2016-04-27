#ifndef RANDOMDESICIONTREE_H
#define RANDOMDESICIONTREE_H

#include <dirent.h>
#include <iostream>
#include <time.h>

#include "include/Util.h"
#include "include/PixelCloud.h"
#include "include/RDFParams.h"

#define MIN_ENTROPY 0.05

struct Node
{
    qint16 m_tau;
    Coord m_teta1, m_teta2;
    quint32 m_id;
    bool m_isLeaf;
    cv::Mat m_hist;
    Node(quint32 id, bool isLeaf):
        m_id(id), m_isLeaf(isLeaf)
    {
    }
};

using TreeNodes = std::vector<Node*>;

struct DataSet
{
    std::vector<cv::Mat> m_trainImagesVector;
    std::vector<cv::Mat> m_testImagesVector;
    std::vector<QString> m_testlabels;
    std::vector<QString> m_trainlabels;

};

class RandomDecisionForest;

class RandomDecisionTree : public QObject
{
    Q_OBJECT

public:
    RandomDecisionTree(RandomDecisionForest *DF) : m_DF(DF) { }

    RandomDecisionForest *m_DF;
    int m_depth;
    int m_numOfLeaves;
    int m_maxDepth;
    int m_probe_distanceX, m_probe_distanceY;

    TreeNodes m_nodes;

    void train();
    void constructTree(Node& root, PixelCloud &pixels);

    inline void setMaxDepth(int max_depth)
    {
        m_maxDepth = max_depth;
        m_nodes.resize((1 << m_maxDepth)-1);
    }

    inline void setProbeDistanceX(int probe_distanceX ) { m_probe_distanceX = probe_distanceX; }
    inline void setProbeDistanceY(int probe_distanceY ) { m_probe_distanceY = probe_distanceY; }

    inline void setMinimumLeafPixelCount(unsigned int min_leaf_pixel_count) { m_minLeafPixelCount = min_leaf_pixel_count; }

    void tuneParameters(std::vector<Pixel*>& parentPixels, Node& parent);

    inline bool isLeft(Pixel* p, Node& node, cv::Mat& img)
    {
        qint16 new_teta1R = node.m_teta1.m_dy + p->position.m_dy;
        qint16 new_teta1C = node.m_teta1.m_dx + p->position.m_dx;
        qint16 intensity1 = img.at<uchar>(new_teta1R,new_teta1C);

        qint16 new_teta2R = node.m_teta2.m_dy + p->position.m_dy ;
        qint16 new_teta2C = node.m_teta2.m_dx + p->position.m_dx ;
        qint16 intensity2 = img.at<uchar>(new_teta2R,new_teta2C);

        return intensity1 - intensity2 <= node.m_tau;
    }

    inline Node* getLeafNode(const DataSet &DS, Pixel*px, int nodeId)
    {
        Node* root = m_nodes[nodeId];
        assert( root != NULL);
        if(root->m_isLeaf)
        {
            // qDebug()<<"LEAF REACHED :"<<root.id;
            return root;
        }
        cv::Mat img = DS.m_testImagesVector[px->imgInfo->sampleId];
        int childId = root->m_id *2 ;
        //qDebug()<<"LEAF SEARCH :"<<root.id << " is leaf : " << root.isLeaf;
        if(!isLeft(px,*root,img))
            ++childId;
        return getLeafNode(DS, px,childId-1);
    }

    bool isPixelSizeConsistent();

    void toString();

private:
    quint32 m_minLeafPixelCount;
    PixelCloud m_pixelCloud;

    void subSample();

    inline void divide(const DataSet& DS, const PixelCloud& parentPixels, std::vector<Pixel*>& left, std::vector<Pixel*>& right, Node& parent)
    {
        for (auto px : parentPixels)
        {
            auto img = DS.m_trainImagesVector[px->imgInfo->sampleId];
            (isLeft(px, parent, img) ? left : right).push_back(px);
        }
    }

    void printPixelCloud();
    void printPixel(Pixel* px);
    void printNode(Node& node);
};

#endif // RANDOMDESICIONTREE_H
