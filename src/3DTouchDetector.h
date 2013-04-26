#ifndef _3DTouchDetectorGuard_H_
#define _3DTouchDetectorGuard_H_

#define AVG_PLUGIN
#include <api.h>

#include <graphics/Bitmap.h>
#include <base/IPlaybackEndListener.h>

#include <player/IInputDevice.h>
#include <player/TrackerInputDeviceBase.h>
#include <player/MultitouchInputDevice.h>

#include "DepthTouchProcessor.h"

namespace avg {

class AVG_PLUGIN DepthTouchDetector: public TrackerInputDeviceBase,
        public IPlaybackEndListener
{
public:
    typedef boost::shared_ptr<DepthTouchDetector> DepthTouchInputDevicePtr;
    DepthTouchDetector(OniCameraPtr cam, const std::string& devName="KinectInput");
    virtual ~DepthTouchDetector();

    void onPlaybackEnd();

    void setBackground();
    void saveDebugImages();

    virtual std::vector<EventPtr> pollEvents();

    virtual void updateBlobs(BlobVectorPtr pBlobs, BlobType type, long long time);

    void updateBlobs(BlobVectorPtr pBlobs, BlobType type, long long time, BitmapPtr depth);

protected:
    OniCameraPtr m_pCamera;

private:
    MutexPtr m_pMutex;
    boost::thread* m_pProcessorThread;
    DepthTouchProcessor::CQueuePtr m_pCmdQueue;
    BitmapPtr m_pDepth;
    int m_currentID;
};

}//End namespace avg


#endif

