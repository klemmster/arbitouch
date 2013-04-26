/*************************************************************************
 * Project: OniPlugin
 *************************************************************************
 * : Richy $
 * : Richard Klemm $
 *************************************************************************
 *
 * Copyright 2011
 *
 ************************************************************************/
#include "DepthTouchProcessor.h"
#include "3DTouchDetector.h"

#include <base/OSHelper.h>
#include <graphics/OGLImagingContext.h>
#include <algorithm>
#include <boost/progress.hpp>

using std::max_element;

namespace avg {

DepthTouchProcessor::DepthTouchProcessor(CQueue& CmdQ,const std::string& threadName,
        OniCameraPtr cam, DepthTouchDetector * pTarget, MutexPtr pMutex, TrackerConfig& config):
    WorkerThread<DepthTouchProcessor>(threadName, CmdQ),
    m_pCamera(cam),
    m_pTarget(pTarget),
    m_pMutex(pMutex)
{
    m_pConfig = TrackerConfigPtr(new TrackerConfig(config));
}

DepthTouchProcessor::~DepthTouchProcessor()
{
    //dtor
}

bool DepthTouchProcessor::init(){
    m_pImagingContext = new OGLImagingContext();
    m_Size = IntPoint(640, 480);
    m_pErosionFilter = FilterPtr(new FilterErosion());
    m_pDilationFilter = FilterPtr(new FilterDilation());
    m_pBlurFilter = FilterPtr(new GPUBlurFilter(m_Size, I16, I16, 1.2, true));
    m_doWork = false;
    return true;
}

void DepthTouchProcessor::deinit(){
}

bool DepthTouchProcessor::work(){
    if(m_doWork){
        m_pBitmap = m_pCamera->getImage(true);
        if(m_pBitmap.get() && m_pBackground.get()){
            BitmapPtr binarized = subtractFromBack();
            m_pErosionFilter->applyInPlace(binarized);
            m_pErosionFilter->applyInPlace(binarized);
            m_pErosionFilter->applyInPlace(binarized);
            m_pErosionFilter->applyInPlace(binarized);

            m_pDilationFilter->applyInPlace(binarized);
            m_pDilationFilter->applyInPlace(binarized);
            long long time = TimeSource::get()->getCurrentMillisecs();
            calcBlobs(binarized, time);
        }
    }else{
        msleep(250);
    }
    return true;
}

void DepthTouchProcessor::setBackground(){
    AVG_TRACE(Logger::PLUGIN, "Set Background Image");
    m_doWork = false;

    BitmapPtr reference = m_pCamera->getImage(true);
    unsigned short * refPixels = (unsigned short*) reference->getPixels();

    int maxSize = m_Size.x*m_Size.y;

    //Initialize Histogramm Vector
    std::vector< std::vector<short> > hist(maxSize);
    for(int i=0; i<maxSize; i++){
        hist.push_back(std::vector<short>());
    }

    unsigned int pointerPos = 0;
    for(int i = 0; i<40; i++){
        AVG_TRACE(Logger::PLUGIN, "Processing Frame #" << i);
        BitmapPtr frame = m_pCamera->getImage(true);
        unsigned short * framePixels = (unsigned short *)frame->getPixels();
        //Bitmap* diff = reference->subtract(frame.get());
        //diffpixels = (unsigned short * ) diff->getPixels();
        for(int y=0; y<m_Size.y; y++){
            for( int x=0; x < m_Size.x; x++){
                pointerPos =  (y*m_Size.x) + x;
                hist[pointerPos].push_back(refPixels[pointerPos] - framePixels[pointerPos]);
            }
        }
    }
    //TODO: Sane treshold
    unsigned short treshold = 5;
    unsigned short * maxDistPixels = new unsigned short[m_Size.x*m_Size.y];
    for(int pos=0; pos < m_Size.x * m_Size.y; pos++){
        short stdDev = *max_element(hist[pos].begin(), hist[pos].end());
        if(refPixels[pos] <= (stdDev+treshold)){
            short val = refPixels[pos] - treshold;
            if (val > 0){
                maxDistPixels[pos] = refPixels[pos] - treshold;
            }else{
                maxDistPixels[pos] = refPixels[pos];
            }
        }else{
            maxDistPixels[pos] = refPixels[pos] - stdDev - treshold;
        }
    }

    m_pBackground = BitmapPtr(new Bitmap(m_Size, I16,
            (unsigned char *)maxDistPixels, m_Size.x*2, false));

    m_doWork = true;
    AVG_TRACE(Logger::PLUGIN, "Done setting Background Image");
}

void DepthTouchProcessor::saveDebugImages(){
    if(m_pBitmap.get() && m_pBackground.get()){

        m_pBackground->save("Background.tif");
        m_pBitmap->save("Depth.tif");

        int length = 640*480;
        unsigned short newPixels[length];
        unsigned short * origPixels = (unsigned short*) m_pBitmap->getPixels();
        for(int i = 0; i<length; i++){
            newPixels[i] = (short)(origPixels[i] << 2)*1.6;
        }
        Bitmap temp(m_Size, I16, (unsigned char *)newPixels, m_Size.x*2, true);
        temp.save("Corrected.png");

        BitmapPtr binarized = subtractFromBack();
        m_pErosionFilter->applyInPlace(binarized);
        m_pErosionFilter->applyInPlace(binarized);
        m_pErosionFilter->applyInPlace(binarized);
        m_pErosionFilter->applyInPlace(binarized);
        binarized->save("Eroded.tif");

        m_pDilationFilter->applyInPlace(binarized);
        m_pDilationFilter->applyInPlace(binarized);
        binarized->save("Dilated.tif");
    }
}

BitmapPtr DepthTouchProcessor::subtractFromBack(){
    BitmapPtr result = BitmapPtr(new Bitmap(m_Size, I8));
    if(m_pBitmap.get() && m_pBackground.get()){
        unsigned char * resultPixels = result->getPixels();
        unsigned short * maxValPixels = (unsigned short *)m_pBackground->getPixels();
        unsigned short * framePixels = (unsigned short *)m_pBitmap->getPixels();

        for(int i=0; i<m_Size.x*m_Size.y; i++){
            const unsigned short * framePixel = &framePixels[i];
            const unsigned short * maxPixel = &maxValPixels[i];
            //TODO: Adjustable Min Pixel
            if( *framePixel < *maxPixel && *framePixel > (*maxPixel - 15)){
                resultPixels[i] = 255;
            }else if( *framePixel < *maxPixel-95 && *framePixel > (*maxPixel - 110)){
                resultPixels[i] = 210;
            }else if( *framePixel < *maxPixel -185 && *framePixel > (*maxPixel - 200)){
                resultPixels[i]= 170;
            }else{
                resultPixels[i] = 0;
            }
        }
    }
    return result;
}

void DepthTouchProcessor::calcBlobs(BitmapPtr pTouchBmp, long long time){
    BlobVectorPtr pBlobs;
    pBlobs = findConnectedComponents(pTouchBmp, 235);
//    m_pTarget->updateBlobs(pBlobs, TOUCH_BLOB, time, pTouchBmp);
    BlobVectorPtr pBlobs2 = findConnectedComponents(pTouchBmp, 200, 230);
    BlobVectorPtr pBlobs3 = findConnectedComponents(pTouchBmp, 160, 180);
    pBlobs->insert(pBlobs->end(), pBlobs2->begin(), pBlobs2->end());
    pBlobs->insert(pBlobs->end(), pBlobs3->begin(), pBlobs3->end());
    BlobVectorPtr result = findRelevandBlobs(pBlobs);
    m_pTarget->updateBlobs(result, TOUCH_BLOB, time, pTouchBmp);
}


//TODO: Stuff copied from TrackerThread for now, cleanup
BlobVectorPtr DepthTouchProcessor::findRelevandBlobs(BlobVectorPtr pBlobs){
    std::string sConfigPrefix = "/tracker/touch/";
    int minArea = m_pConfig->getIntParam(sConfigPrefix+"areabounds/@min");
    int maxArea = m_pConfig->getIntParam(sConfigPrefix+"areabounds/@max");
    double minEccentricity = m_pConfig->getDoubleParam(sConfigPrefix+"eccentricitybounds/@min");
    double maxEccentricity = m_pConfig->getDoubleParam(sConfigPrefix+"eccentricitybounds/@max");

    BlobVectorPtr pRelevantBlobs(new BlobVector());
    for(BlobVector::iterator it = pBlobs->begin(); it != pBlobs->end(); ++it) {
        if (isRelevant(*it, minArea, maxArea, minEccentricity, maxEccentricity)) {
            pRelevantBlobs->push_back(*it);
        }
        if (pRelevantBlobs->size() > 50) {
            break;
        }
    }
    return pRelevantBlobs;
}

inline bool isInbetween(int x, int min, int max)
{
    return (x >= min) && (x <= max);
}

bool DepthTouchProcessor::isRelevant(BlobPtr pBlob, int minArea, int maxArea,
        double minEccentricity, double maxEccentricity)
{
    bool res;
//    std::cerr << "Area: " << pBlob->getArea() << std::endl;
//    std::cerr << "Min: " << minArea <<  "Max: " << maxArea << std::endl;
    res = isInbetween(pBlob->getArea(), minArea, maxArea) &&
            isInbetween(pBlob->getEccentricity(), minEccentricity, maxEccentricity);
    return res;
}

}//End namespace avg
