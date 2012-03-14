#ifndef YGG_TRANSPORT_IMPL_HPP
#define YGG_TRANSPORT_IMPL_HPP

#include "yggTypeRegistry.hpp"
#include <cassert>


namespace ygg
{

inline
Transport::Transport()
 : mState(DEVICE_STOPPED),
   mReadChecksum(0),
   mWriteChecksum(0)
{}


inline bool 
Transport::isFunctional() const 
{
    return !(isError() || isStopped());
}

inline void 
Transport::setFunctional()
{
    mState = DEVICE_FUNCTIONAL;
}

inline bool 
Transport::isError() const 
{
    return mState == DEVICE_ERROR;
}

inline void 
Transport::setError() 
{
    mState = DEVICE_ERROR;
}

inline bool 
Transport::isStopped() const 
{
    return mState == DEVICE_STOPPED;
}

inline void 
Transport::setStopped() 
{
    mState = DEVICE_STOPPED;
}

inline bool 
Transport::isWaitSync() const
{
    return mState == DEVICE_WAITING_SYNC;
}

inline void 
Transport::setWaitSync()
{
    mState = DEVICE_WAITING_SYNC;
}

inline void 
Transport::serialize(const TypeBase* d)
{
    //assert(d && d->desc());
    UnitType typeId = d->id();
    // write the synchronization byte
    write((UnitType)SYNC_BYTE);
    // write the type
    write(typeId);
    // write the checksum
    UnitType cs = 255 - typeId - SYNC_BYTE;
    write(cs);
    // reset the checksum, we will be using it when dumping the object
    mWriteChecksum = 0;
    d->write(*this);
    // write the calculated checksum
    write(mWriteChecksum);
}

inline Transport::UnitType 
Transport::readObjectType()
{
    UnitType s, t, cs;
    // read the first byte, we hope this is the sync.
    read(s);
    while (isWaitSync()) {
        if(s == SYNC_BYTE) {
            // ok check the next one, it should be the data type byte
            read(t);
            if(!TypeRegistry::isForeignTypeEnabled(t)) {
                // nope, continue search
                s = t;
                continue;
            }
            // ok, so far so good, read the checksum
            read(cs);
            if(cs + s + t != 255) {
                // checksum didn't match, continue from here
                s = cs;
                continue;
            }
            // we are good to go!
            setFunctional();
            break;
        }
        read(s);
    }
    return t;
}

inline void 
Transport::deserialize(TypeBase*& d)
{
    d = NULL;
    UnitType fType = readObjectType();
    // if we reached here then we have a sync!
    assert(!isWaitSync());
    d = buildObject(fType);
    // waiting for the next object no matter the previous was successfull or not...
    setWaitSync();
}

inline TypeBase* 
Transport::buildObject(UnitType fType)
{
    mReadChecksum = 0;
    // construct the object
    TypeBase* d = TypeRegistry::instantiateForeignType(fType);
    // make sure the type was valid.. TBD: do a proper handling...
    if(d == NULL) {
        return NULL;
    } else {
        // read the object
        d->read(*this);
        // read the checksum
        ChecksumType computedChecksum = mReadChecksum;
        ChecksumType readChecksum;
        read(readChecksum);
        // and check the data 
        if(readChecksum != computedChecksum) {
            delete d;
            return NULL;
        }
    }
    assert(d);
    return d;
}

inline Transport::ChecksumType
Transport::calculateChecksumN(const void* ptr, uint32_t size)
{
    ChecksumType checksum = 0;
    uint8_t* bptr = (uint8_t*)ptr;
    for(uint32_t i = 0; i < size; ++i) {
        checksum += bptr[i];
    }
    return checksum;
}

inline Transport::ChecksumType
Transport::calculateChecksum8(const void* ptr)
{
    uint8_t* bptr = (uint8_t*)ptr;
    ChecksumType checksum = bptr[0];
    return checksum;
}

inline Transport::ChecksumType
Transport::calculateChecksum16(const void* ptr)
{
    uint8_t* bptr = (uint8_t*)ptr;
    ChecksumType checksum = bptr[0] + bptr[1];
    return checksum;
}

inline Transport::ChecksumType
Transport::calculateChecksum32(const void* ptr)
{
    uint8_t* bptr = (uint8_t*)ptr;
    ChecksumType checksum = bptr[0] + bptr[1] + bptr[2] + bptr[3];
    return checksum;
}

inline Transport::ChecksumType
Transport::calculateChecksum64(const void* ptr)
{
    uint8_t* bptr = (uint8_t*)ptr;
    ChecksumType checksum = bptr[0] + bptr[1] + bptr[2] + bptr[3] + 
                            bptr[4] + bptr[5] + bptr[6] + bptr[7];
    return checksum;
}

////////////////////////////////////////////////////////
// Writing methods                                    //
////////////////////////////////////////////////////////
inline void
Transport::write(uint64_t intd)
{
    fixEndianness64(&intd);
    write(&intd, sizeof(uint64_t));
    mWriteChecksum += calculateChecksum64(&intd);
}

inline void
Transport::write(int64_t intd)
{
    fixEndianness64(&intd);
    write(&intd, sizeof(int64_t));
    mWriteChecksum += calculateChecksum64(&intd);
}

inline void
Transport::write(uint32_t intd)
{
    fixEndianness32(&intd);
    write(&intd, sizeof(uint32_t));
    mWriteChecksum += calculateChecksum32(&intd);
}

inline void
Transport::write(int32_t intd)
{
    fixEndianness32(&intd);
    write(&intd, sizeof(int32_t));
    mWriteChecksum += calculateChecksum32(&intd);
}

inline void
Transport::write(uint16_t intd)
{
    fixEndianness16(&intd);
    write(&intd, sizeof(uint16_t));
    mWriteChecksum += calculateChecksum16(&intd);
}

inline void
Transport::write(int16_t intd)
{
    fixEndianness16(&intd);
    write(&intd, sizeof(int16_t));
    mWriteChecksum += calculateChecksum16(&intd);
}

inline void
Transport::write(uint8_t intd)
{
    write(&intd, sizeof(uint8_t));
    mWriteChecksum += calculateChecksum8(&intd);
}

inline void
Transport::write(int8_t intd)
{
    write(&intd, sizeof(int8_t));
    mWriteChecksum += calculateChecksum8(&intd);
}


inline void
Transport::write(float floatd)
{
    fixEndianness32(&floatd);
    write(&floatd, sizeof(float));
    mWriteChecksum += calculateChecksum32(&floatd);
}

inline void
Transport::write(double doubled)
{
    fixEndianness64(&doubled);
    write(&doubled, sizeof(double));
    mWriteChecksum += calculateChecksum64(&doubled);
}

inline void
Transport::write(const std::string& stringd)
{
    uint32_t stringd_len = stringd.length();
    writeChecksumed(stringd_len);
    write(stringd.c_str(), stringd_len);
    mWriteChecksum += calculateChecksumN(stringd.c_str(), stringd_len);
}


////////////////////////////////////////////////////////
// Reading methods                                    //
////////////////////////////////////////////////////////
inline void
Transport::read(uint64_t& intd)
{
    read(&intd, sizeof(uint64_t));
    mReadChecksum += calculateChecksum64(&intd);
    fixEndianness64(&intd);
}

inline void
Transport::read(int64_t& intd)
{
    read(&intd, sizeof(int64_t));
    mReadChecksum += calculateChecksum64(&intd);
    fixEndianness64(&intd);
}

inline void
Transport::read(uint32_t& intd)
{
    read(&intd, sizeof(uint32_t));
    mReadChecksum += calculateChecksum32(&intd);
    fixEndianness32(&intd);
}

inline void
Transport::read(int32_t& intd)
{
    read(&intd, sizeof(int32_t));
    mReadChecksum += calculateChecksum32(&intd);
    fixEndianness32(&intd);
}


inline void
Transport::read(uint16_t& intd)
{
    read(&intd, sizeof(uint16_t));
    mReadChecksum += calculateChecksum16(&intd);
    fixEndianness16(&intd);
}

inline void
Transport::read(int16_t& intd)
{
    read(&intd, sizeof(int16_t));
    mReadChecksum += calculateChecksum16(&intd);
    fixEndianness16(&intd);
}

inline void
Transport::read(uint8_t& intd)
{
    read(&intd, sizeof(uint8_t));
    mReadChecksum += calculateChecksum8(&intd);
}

inline void
Transport::read(int8_t& intd)
{
    read(&intd, sizeof(int8_t));
    mReadChecksum += calculateChecksum8(&intd);
}

inline void
Transport::read(float& floatd)
{
    read(&floatd, sizeof(float));
    mReadChecksum += calculateChecksum32(&floatd);
    fixEndianness32(&floatd);
}

inline void
Transport::read(double& doubled)
{
    read(&doubled, sizeof(double));
    mReadChecksum += calculateChecksum64(&doubled);
    fixEndianness64(&doubled);
}

inline void
Transport::read(std::string& stringd)
{
    uint32_t stringd_len;
    readChecksumed(stringd_len);
    if(isWaitSync()) {
        return;
    }
    char* buf = new char[stringd_len];
    read(buf, stringd_len);
    mReadChecksum += calculateChecksumN(buf, stringd_len);
    stringd.assign(buf, stringd_len);
    delete buf;
}



template <class T>
void
Transport::readChecksumed(T& data) 
{
    // save the current checksum
    ChecksumType curChecksum = mReadChecksum;
    // reset it
    mReadChecksum = 0;
    // read the data
    read(data);
    // read and get the calculated chechsums
    ChecksumType computedChecksum = mReadChecksum;
    ChecksumType readChecksum;
    read(readChecksum);
    // restore the saved checksum
    mReadChecksum += curChecksum;
    // check the checksum and change the device state if needed 
    if(readChecksum != computedChecksum) {
        setWaitSync();
    }
}

template <class T>
void
Transport::writeChecksumed(const T& data) 
{
    // save the current checksum
    ChecksumType curChecksum = mWriteChecksum;
    // reset it
    mWriteChecksum = 0;
    // write the data
    write(data);
    // write the checksum
    write(mWriteChecksum);
    // restore the saved checksum
    mWriteChecksum += curChecksum;
}


////////////////////////////////////////////////////////
// Endianness                                         //
////////////////////////////////////////////////////////
template <ConfigEndianness E, int L> 
inline void
Transport::fixEndianness(void*)
{
}

template <> 
inline void
Transport::fixEndianness<ENDIAN_SWAP, 2>(void* ptr)
{
    uint16_t& v = *(uint16_t*)ptr;
    v = (v>>8) | 
        (v<<8);
}

template <> 
inline void
Transport::fixEndianness<ENDIAN_SWAP, 4>(void* ptr)
{
    uint32_t& v = *(uint32_t*)ptr;
    v = (v>>24) | 
        ((v<<8) & 0x00FF0000) |
        ((v>>8) & 0x0000FF00) |
        (v<<24);
}

template <> 
inline void
Transport::fixEndianness<ENDIAN_SWAP, 8>(void* ptr)
{
    uint64_t& v = *(uint64_t*)ptr;
    v = (v>>56) | 
        ((v<<40) & 0x00FF000000000000) |
        ((v<<24) & 0x0000FF0000000000) |
        ((v<<8)  & 0x000000FF00000000) |
        ((v>>8)  & 0x00000000FF000000) |
        ((v>>24) & 0x0000000000FF0000) |
        ((v>>40) & 0x000000000000FF00) |
        (v<<56);
}

inline void
Transport::swap(Transport& transport) 
{
    std::swap(mState, transport.mState);
    std::swap(mWriteChecksum, transport.mWriteChecksum);
    std::swap(mReadChecksum, transport.mReadChecksum);
}

template <typename C, typename D>
ConfiguredTransport<C,D>::ConfiguredTransport(D* device)
 : mDevice(device)
{
}

template <typename C, typename D>
void 
ConfiguredTransport<C,D>::fixEndianness16(void* ptr)
{
    fixEndianness<C::Endianness,2>(ptr);
}

template <typename C, typename D>
void 
ConfiguredTransport<C,D>::fixEndianness32(void* ptr)
{
    fixEndianness<C::Endianness,4>(ptr);
}

template <typename C, typename D>
void 
ConfiguredTransport<C,D>::fixEndianness64(void* ptr)
{
    fixEndianness<C::Endianness,8>(ptr);
}

template <typename C, typename D>
void
ConfiguredTransport<C,D>::start()
{
    if(mDevice && mDevice->isOpen()) {
        setWaitSync();
    } else {
        setError();
    }
}

template <typename C, typename D>
void
ConfiguredTransport<C,D>::stop()
{
    setStopped();
}

template <typename C, typename D>
void
ConfiguredTransport<C,D>::swap(ConfiguredTransport<C,D>& ctransport) 
{
    stop();
    std::swap(mDevice, ctransport.mDevice);
    Transport::swap(ctransport);
}

////////////////////////////////////////////////////////
// Low level methods                                  //
////////////////////////////////////////////////////////
template <typename C, typename D>
void
ConfiguredTransport<C,D>::write(const void* ptr, uint32_t size)
{
    if(isFunctional() && !mDevice->write((uint8_t*)ptr, size)) {
        setError();
    }
}

template <typename C, typename D>
void
ConfiguredTransport<C,D>::read(void* ptr, uint32_t size) 
{
    if(isFunctional() && !mDevice->read((uint8_t*)ptr, size)) {
        setError();
    }
}

} // namespace ygg

#endif //YGG_TRANSPORT_IMPL_HPP
