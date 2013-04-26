#include "3DTouchDetector.h"

#include <player/Player.h>
#include <player/TouchEvent.h>
#include <player/TrackerTouchStatus.h>
#include <base/Logger.h>

#include <boost/foreach.hpp>


namespace avg{

DepthTouchDetector::DepthTouchDetector(OniCameraPtr cam, const std::string& devName):
    TrackerInputDeviceBase(devName),
    m_pCamera(cam)
{
    m_DisplayROI = DRect(0.0, 0.0, 0.0, 0.0);
    m_pCmdQueue = DepthTouchProcessor::CQueuePtr(new DepthTouchProcessor::CQueue);
    m_pMutex = MutexPtr(new boost::mutex);
    m_pProcessorThread = new boost::thread(DepthTouchProcessor(*m_pCmdQueue,
            std::string("DepthProcessorThread"), cam, this, m_pMutex, m_TrackerConfig));
    m_currentID = 0;
    Player::get()->addInputDevice(IInputDevicePtr(this));
}

DepthTouchDetector::~DepthTouchDetector(){

}

void DepthTouchDetector::onPlaybackEnd(){
    AVG_TRACE(Logger::PLUGIN, "End Playback 3DTouch");
    m_pCmdQueue->pushCmd(boost::bind(&DepthTouchProcessor::stop,_1));
    m_pProcessorThread->join();
}

void DepthTouchDetector::setBackground(){
    m_pCmdQueue->pushCmd(boost::bind(&DepthTouchProcessor::setBackground, _1));
}

void DepthTouchDetector::saveDebugImages(){
    m_pCmdQueue->pushCmd(boost::bind(&DepthTouchProcessor::saveDebugImages, _1));
}

std::vector<EventPtr> DepthTouchDetector::pollEvents(){
    std::vector<EventPtr> pTouchEvents;
    std::vector<EventPtr> pTrackEvents;
    pollEventType(pTouchEvents, m_TouchEvents, CursorEvent::TOUCH);
    pollEventType(pTrackEvents, m_TrackEvents, CursorEvent::TRACK);
    boost::mutex::scoped_lock lock(*m_pMutex);
    for(std::vector<EventPtr>::iterator it = pTouchEvents.begin(); it != pTouchEvents.end(); ++it){
        if(m_pDepth.get()){
            TouchEventPtr event = boost::dynamic_pointer_cast<TouchEvent>(*it);
            unsigned int pos = (unsigned int)(event->getPos().x + (event->getPos().y * 640));
            unsigned short pixel = m_pDepth->getPixels()[pos];
            event->setDepth(pixel);
        }
    }
    for(std::vector<EventPtr>::iterator it = pTrackEvents.begin(); it != pTrackEvents.end(); ++it){
        if(m_pDepth.get()){
            boost::dynamic_pointer_cast<TouchEvent>(*it)->setDepth(200);
        }
    }
    pTouchEvents.insert(pTouchEvents.end(), pTrackEvents.begin(), pTrackEvents.end());
    return pTouchEvents;
}

void DepthTouchDetector::updateBlobs(BlobVectorPtr pBlobs, BlobType type, long long time){
    if(type == TOUCH_BLOB){
//        for(BlobVector::iterator it = pBlobs->begin(); it != pBlobs->end(); ++it){
//            std::cerr << "Eccentricity: " << (*it)->getEccentricity() << std::endl;
//        }
        trackBlobIDs(pBlobs, time, true);
//        AVG_TRACE(Logger::PLUGIN, "Touch Blobs: " << pBlobs->size());
    }else if(type == TRACK_BLOB){
        trackBlobIDs(pBlobs, time, false);
//        AVG_TRACE(Logger::PLUGIN, "Track Blobs: " << pBlobs->size());
    }
}

void DepthTouchDetector::updateBlobs(BlobVectorPtr pBlobs, BlobType type, long long time,
        BitmapPtr depth)
{
    m_pDepth = depth;
    updateBlobs(pBlobs, type, time);
}

}//end namespace avg
