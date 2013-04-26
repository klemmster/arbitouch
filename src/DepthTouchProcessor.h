/*************************************************************************
 * Project: Bonnie - Server
 * Function:
 *************************************************************************
 * : Richy $
 * : Richard Klemm $
 *************************************************************************
 *
 * Copyright 2009 by Coding Reality GbR
 *
 ************************************************************************/
#ifndef _DEPTHTOUCHPROCESSOR_H_
#define _DEPTHTOUCHPROCESSOR_H_

#define AVG_PLUGIN
#include <api.h>

#include <base/TimeSource.h>
#include <base/WorkerThread.h>
#include <base/Queue.h>

#include <imaging/Blob.h>
#include <imaging/Camera.h>

#include <graphics/Bitmap.h>
#include <graphics/FilterErosion.h>
#include <graphics/FilterDilation.h>
#include <graphics/GPUBlurFilter.h>

#include <player/TrackerInputDeviceBase.h>

#include "../../openNICam_Plugin/src/OniCamera.h"


namespace avg{

    class DepthTouchDetector;
    class OGLImagingContext;

class AVG_API DepthTouchProcessor : public WorkerThread<DepthTouchProcessor>
{
    public:
        DepthTouchProcessor(CQueue& CmdQ,const std::string& threadName, OniCameraPtr camera,
                DepthTouchDetector *pTarget, MutexPtr pMutex, TrackerConfig& config);
        virtual ~DepthTouchProcessor();

        bool init();
        void deinit();
        bool work();

        void setBackground();
        void saveDebugImages();

    protected:

        BlobVectorPtr findRelevandBlobs(BlobVectorPtr pBlobs);
        bool isRelevant(BlobPtr pBlob, int minArea, int maxArea,
        double minEccentricity, double maxEccentricity);

    private:
        BitmapPtr subtractFromBack();
        void calcBlobs(BitmapPtr pTouchBmp, long long time);

        BitmapPtr m_pBitmap;
        BitmapPtr m_pBackground;

        OniCameraPtr m_pCamera;

        IntPoint m_Size;

        FilterPtr m_pErosionFilter;
        FilterPtr m_pDilationFilter;
        FilterPtr m_pBlurFilter;

        bool m_doWork;

        MutexPtr m_pMutex;
        DepthTouchDetector *m_pTarget;

        OGLImagingContext* m_pImagingContext;
        TrackerConfigPtr m_pConfig;
};

}//end namespace avg

#endif // _DEPTHTOUCHPROCESSOR_H_
