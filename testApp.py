#!/usr/bin/env python
# -*- coding: utf-8 -*-

from libavg import avg, AVGApp

g_Player = avg.Player.get()

class TestPlugin(AVGApp):

    plugin = None
    manager = None
    oniDev = None

    def init(self):
        g_Player.loadPlugin('onicam')
        g_Player.loadPlugin('DepthTouchPlugin')
        root = g_Player.getRootNode()
        root.connectEventHandler(avg.CURSORDOWN, avg.TOUCH, self, self._onTouch)
        root.connectEventHandler(avg.KEYDOWN, avg.NONE,self, self.__onKeyDown)

        cam1Container = avg.DivNode(size=(640, 480))
        node = OniCam.OniCameraNode(size=(640, 480))
        node.activateCamera(OniCam.ONI_RGB_CAMERA)
        self.colorCam = node
        cam1Container.appendChild(node)

        cam2Container = avg.DivNode(size=(640, 480), pos=(640, 0))
        self.node = OniCam.OniCameraNode(size=(640, 480))
        self.node.activateCamera(OniCam.ONI_DEPTH_CAMERA)
        cam2Container.appendChild(self.node)

        self._parentNode.appendChild(cam1Container)
        self._parentNode.appendChild(cam2Container)
        print "Enable Touch Module"
        self.touchModule = DepthTouchModule.DepthTouchPlugin(self.node.getCamera())
        self.touchModule.setBackground()
        self.blur = 0.0
        self.rects = {}

    def _onTouch(self, event):
        print "Touch"
        print "Depth: " + str(event.depth)

    def __onKeyDown(self, event):
        if event.keystring == 'b':
            print 'Set Background'
            self.touchModule.setBackground()
        elif event.keystring == 'd':
            print 'Save Debug Infoos'
            self.touchModule.saveDebugImages()
            self.colorCam.save("color.png")
        elif event.keystring == 'v':
            print "create Filter"
            self.touchModule.createBlurFilter(self.blur)
        elif event.keystring == 'j':
            print "inc blur: "
            self.blur += 0.1
            print self.blur
        elif event.keystring =='k':
            print "dec blur: "
            self.blur -= 0.1
            print self.blur

if __name__ == '__main__':
    TestPlugin.start(resolution=(1280, 960))
