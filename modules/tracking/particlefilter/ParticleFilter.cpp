#include "precompiled.h"

#include <time.h>

#include "tracking/particlefilter/ParticleFilter.h"
#include "Util.h"
#include "Target.h"
#include "tracking/particlefilter/RectangleParticle.h"

ParticleFilter::ParticleFilter(
        int frameWidth, int frameHeight, int nParticles, int nIters,
        int particleWidth, int particleHeight, int histSize, Target *target
        )
{
    type = Rectangle;
    m_target = target;
    img_height = frameHeight;
    img_width = frameWidth;
    m_num_particles_to_display = 0;
    setNumParticles(nParticles);
    setNumIters(nIters);
    setParticleWidth(particleWidth);
    setParticleHeight(particleHeight);
    setParticlesToDisplay(m_num_particles_to_display);
    setHistSize(histSize);
    m_distortRange = 5;
    srand(time(nullptr));
    initializeParticles();
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    m_RandomGen = std::mt19937(seed);
}

int ParticleFilter::getRatioOfTop(int count)
{
    float total_weight = 0;
    for (int i = 0; i < count; ++i)
        total_weight += m_particles[i]->weight();
    return total_weight * 100;
}

void ParticleFilter::setVideoReader(VideoReader *videoReader) {m_VideoReader = videoReader; }

void ParticleFilter::processImage()
{
    auto randomParticle = [this](float rand_number) {
        float sum{};
        for (int i = 0; i < m_num_particles; ++i)
        {
            sum += m_particles[i]->weight();
            if (sum >= rand_number)
                return i;
        }
        return 0;
    };

    for (int i = 0; i < m_num_iters; ++i)
    {
        // resample particles
        auto rand_dice = [&]() { return std::uniform_real_distribution<float>(0, 1)(m_RandomGen); };
        for (int j = 0; j < m_num_particles; ++j)
        {
            float rand_number = rand_dice();
            auto *p_to_distort = m_particles[randomParticle(rand_number)];
            int newX, newY;
            distortParticle(p_to_distort, newX, newY);
            m_newCoordinates.push_back(cv::Point(newX, newY));
        }

        // update particles
        for (int i = 0; i < m_num_particles; ++i)
        {
            auto *p_particle = m_particles[i];
            p_particle->setCoordinates(m_newCoordinates[i]);
            p_particle->setWeight(1.0f / m_num_particles);
        }
        m_newCoordinates.clear();

        // update weights
        for(Particle *P : m_particles)
            P->exec(*&m_img);

        // sort descending
        std::sort(std::begin(m_particles), std::end(m_particles), [](Particle * P1, Particle * P2) {
            return P1->weight() > P2->weight();
        });

        // normalize weights
        auto total_weight = sumall(m_particles, [](Particle * P) { return P->weight(); });
        auto nParticles = m_particles.size();
        for (quint32 i = 0; i < nParticles; ++i)
            m_particles[i]->setWeight(m_particles[i]->weight() / total_weight);
    }
    showTopNParticles(m_num_particles_to_display);
}


void ParticleFilter::initializeParticles()
{
    int xRange = (img_width - m_particle_width);
    int yRange = (img_height - m_particle_height);
    auto randX =  [&]() { return std::uniform_int_distribution<int>(1, xRange)(m_RandomGen); };
    auto randY =  [&]() {  return std::uniform_int_distribution<int>(1, yRange)(m_RandomGen); };
    for (int i = 0; i < m_num_particles; ++i)
    {
        int x = randX();
        int y = randY();
        auto weight = 1.0f / m_num_particles;
        Particle *particle = new RectangleParticle(x, y, m_particle_width, m_particle_height, weight, m_target->getHist(),
                                                   m_histSize);
        m_particles.push_back(particle);
    }
}

// TODO: double'dan inte cast edilliyor tekrar?
void ParticleFilter::distortParticle(Particle *p, int &x, int &y)
{
    try
    {
        std::normal_distribution<double> distribution(0, m_distortRange);
        int dx = (int)distribution(m_RandomGen);
        int dy = (int)distribution(m_RandomGen);
        x = p->x();
        y = p->y();
        int newx = x + dx;
        int newy = y + dy;
        if (newx < img_width - m_particle_width && newx > 0)
            x = newx;
        if (newy < img_height - m_particle_width && newy > 0)
            y = newy;
    }
    catch (cv::Exception &e)
    {
        std::cout << "Distribution Error" << e.msg;
    }
}

void ParticleFilter::showParticles()
{
    m_img->copyTo(m_outIMG);
    int x = 0;
    int y = 0;
    for (int i = 0; i < m_num_particles; ++i)
    {
        x += m_particles[i]->x();
        y += m_particles[i]->y();
    }
    x = x / m_num_particles;
    y = y / m_num_particles;
    int x_end = x + m_particle_width;
    int y_end = y + m_particle_height;
    int r = m_particle_width / 2;;
    if (getModelType() == Circle)
        circle(m_outIMG, cvPoint(x + r, y + r), r, cvScalar(130, 0, 0), 1);
    else if (getModelType() == Rectangle)
        rectangle(m_outIMG, cvPoint(x, y), cvPoint(x_end, y_end), cvScalar(130, 0, 0), 1);
    else
        rectangle(m_outIMG, cvPoint(x, y), cvPoint(x_end, y_end), cvScalar(130, 0, 0), 1);
}

void ParticleFilter::showTopNParticles(int count)
{
    m_img->copyTo(m_outIMG);
    if (count > m_num_particles)
        count = m_num_particles;
    else if (count == 0)
        showParticles();
    int x = 0;
    int y = 0;
    for (int i = 0; i < count; ++i)
    {
        x = m_particles[i]->x();
        y = m_particles[i]->y();
        int x_end = x + m_particle_width;
        int y_end = y + m_particle_height;
        rectangle(m_outIMG, cvPoint(x, y), cvPoint(x_end, y_end), cvScalar(130, 0, 0), 1);
    }
}

void ParticleFilter::reInitialiaze()
{
    m_particles.clear();
    initializeParticles();
}

ParticleFilter::~ParticleFilter() {}
