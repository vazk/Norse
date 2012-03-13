#include <QApplication>
#include <QMainWindow>
#include "thoMainWindow.hpp"
#include "thoInputHandler.hpp"
#include "thoSerialization.hpp"

using namespace std;
using thor::sm;
using thor::registry;

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );

    //Create and show the main window
    thor::MainWindow mw;
    mw.show();

    // register used types...
    registry::addType<rat::StrCmdData>("StrCmdData", 1);
    registry::addType<rat::BasicType<uint32_t, 6> >("BasicType2", 1);
    registry::addType<rat::BasicType<int32_t, 3> >("BasicType3", 1);
    registry::addType<rat::LISData>("LISData", 1);
    registry::addType<rat::BasicType<float, 2> >("BasicType4", 1);

    // instantiate the input data handler type
    sm::InputHandler handler(&mw);
    // specifying uart device name and create the device...
    sm::DeviceParams params = { "/dev/ttyUSB0" };
    sm::Device device(params, sm::Device::INOUT);
    if(!device.isOpen()) {
        return 1;
    }
    // setup the transport
    sm::Transport transport(&device);

    // now initialize the log device... 
    sm::DeviceParams lparams= { "logfile.out" };
    sm::Device ldevice(lparams, sm::Device::OUT);
    if(!ldevice.isOpen()) {
        return 1;
    }

    sm::Logger logger(&ldevice);

    // start the service
    sm::startService(transport, handler);
    //sm::startLogger(logger);

    return a.exec();
}
