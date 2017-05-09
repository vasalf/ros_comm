// I'm not pretty sure whether I should place a copyright here or not.

#ifndef ROSCPP_TRANSPORT_INTRAPROCESS_H
#define ROSCPP_TRANSPORT_INTRAPROCESS_H

#include "ros/common.h"
#include "ros/transport/transport.h"

#include <boost/thread/mutex.hpp>
#include <deque>

namespace ros
{

class TransportIntraProcess;
typedef boost::shared_ptr<TransportIntraProcess> TransportIntraProcessPtr;

class ROSCPP_DECL TransportIntraProcess : public Transport
{
public:
  TransportIntraProcess();
  virtual ~TransportIntraProcess();

  void setInterlocutor(TransportIntraProcessPtr& interlocutor);

  void accept(uint8_t* buffer, uint32_t size);
  
  // overrides from Transport
  virtual uint32_t read(uint8_t* buffer, uint32_t size);
  virtual uint32_t write(uint8_t* buffer, uint32_t size);

  virtual void enableWrite();
  virtual void disableWrite();
  virtual void enableRead();
  virtual void disableRead();

  virtual void close();

  virtual std::string getTransportInfo();

  virtual const char* getType() { return "intraprocess"; }
  
private:

  boost::mutex configuration_mutex_;
  TransportIntraProcess interlocutor_;
  bool write_enabled_;
  bool read_enabled_;

  boost::mutex buffer_mutex_;
  std::deque<uint8_t> buffer_;
};

}

#endif //ROSCPP_TRANSPORT_INTRAPROCESS_H
