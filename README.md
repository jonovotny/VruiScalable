# VruiScalable

To Build
--------
* Open up a shell on login001 or login002
* Define $G as the root of this cloned repository
* Load cave, cave-utils/yurt, centos-libs/6.7, cmake/3.2.2, gcc, nvidia-driver, and vrpn modules
* Run ./build

To Run
------
* Open two shells on cave001
* In one, run ./VRDeviceDaemon script
* In the other, run ./MeshViewer, ./3DVisualizer, or any other application script

Files Modified for Scalable Integration
---------------------------------------
* import/vrui/src/BuildRoot/Packages.System
* import/vrui/src/BuildRoot/Packages.Vrui
* import/vrui/src/Vrui/Scalable.cpp
* import/vrui/src/Vrui/Scalable.h
* import/vrui/src/Vrui/VRWindow.cpp
* import/vrui/src/Vrui/VRWindow.h
