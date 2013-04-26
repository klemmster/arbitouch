#include "3DTouchPlugin.h"

#include <wrapper/WrapHelper.h>
#include <wrapper/raw_constructor.hpp>
#include <graphics/Bitmap.h>

#include "3DTouchDetector.h"

using namespace boost::python;

BOOST_PYTHON_MODULE(DepthTouchPlugin)
{

    class_<avg::DepthTouchDetector, boost::shared_ptr<avg::DepthTouchDetector>,
            boost::noncopyable>("DepthTouchPlugin", init<avg::OniCameraPtr>() )
        .def("setBackground", &avg::DepthTouchDetector::setBackground)
        .def("saveDebugImages", &avg::DepthTouchDetector::saveDebugImages)
        ;

}

AVG_PLUGIN_API void registerPlugin()
{
    initDepthTouchPlugin();
    object mainModule(handle<>(borrowed(PyImport_AddModule("__main__"))));
    object DepthTouchDetectorModule(handle<>(PyImport_ImportModule("DepthTouchPlugin")));
    mainModule.attr("DepthTouchModule") = DepthTouchDetectorModule;
}
