#ifndef THO_SERIALIALIZATION_HPP
#define THO_SERIALIALIZATION_HPP

#include "yggManager.hpp"
#include "yggTypes.hpp"
#include "yggQtTraits.hpp"

namespace thor {

class InputHandler;
struct ThorQtConfig
{
    // configuration of the serialization system
    const static ygg::ConfigCommunication   Serialization    = ygg::COMMUNICATION_NONBLOCKING;
    const static ygg::ConfigCommunication   Deserialization  = ygg::COMMUNICATION_NONBLOCKING;
    const static ygg::ConfigEndianness      Endianness       = ygg::ENDIAN_NATIVE;
    const static ygg::ConfigManifest        ManifestRequired = ygg::MANIFEST_REQUIRED;
    // various parameters of the serialization system
    const static int BasePriority = 0;
    const static int InputQueueSize = 10;
    const static int OutputQueueSize = 10;
    const static int ManifestRequestMs = 1000;
};

typedef ygg::Manager<
                     ygg::QtSystemTraits, 
                     InputHandler,  
                     ygg::ConfiguredTransport<ThorQtConfig>, 
                     ThorQtConfig
                   > sm;

}

#endif  //THO_SERIALIALIZATION_HPP
