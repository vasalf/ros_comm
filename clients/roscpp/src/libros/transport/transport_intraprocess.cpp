// I'm not pretty sure whether I should place a copyright here or not.

#include "ros/transport/transport_intraprocess.h"

#include <algorithm>
#include <sstream>

namespace ros
{

TransportIntraProcess::TransportIntraProcess()
  : interlocutor_(NULL),
    write_enabled_(false),
    read_enabled_(true) {}

TransportIntraProcess::~TransportIntraProcess() {}

void TransportIntraProcess::setInterlocutor(TransportIntraProcessPtr& interlocutor)
{
  ROS_ASSERT(interlocutor != NULL);
  {
    boost::mutex::scoped_lock lock(configuration_mutex_);
    ROS_ASSERT(interlocutor_ == NULL);
    interlocutor_ = interlocutor;
  }
  enableWrite();
}

uint32_t TransportIntraProcess::accept(uint8_t* buffer, uint32_t size)
{
  bool read_enabled;
  {
    boost::mutex::scoped_lock conf_lock(configuration_mutex_);
    boost::mutex::scoped_lock buf_lock(buffer_mutex_);
    read_enabled = read_enabled_;
    std::copy(buffer, buffer + size, std::back_inserter(buffer_));
  }
  if (read_enabled && read_cb_)
      read_cb_(shared_from_this());
  return size;
}

int32_t TransportIntraProcess::read(uint8_t* buffer, uint32_t size)
{
  {
    boost::mutex::scoped_lock conf_lock(configuration_mutex_);
    if (!read_enabled_) {
      return 0;
    }
  }

  boost::mutex::scoped_lock buf_lock(buffer_mutex_);
  size = std::min(size, (uint32_t)buffer_.size());
  int32_t ret = size;
  while (size > 0) {
    *(buffer++) = buffer_.front();
    buffer_.pop_front();
    size--;
  }
  return ret;
}

int32_t TransportIntraProcess::write(uint8_t* buffer, uint32_t size)
{
  {
    boost::mutex::scoped_lock conf_lock(configuration_mutex_);
    if (!write_enabled_)
      return 0;
  }

  // interlocutor should not be NULL if write is enabled
  ROS_ASSERT(interlocutor_ != NULL);
  uint32_t ret = interlocutor_->accept(buffer, size);
  return ret;
}

void TransportIntraProcess::enableWrite()
{
  {
    boost::mutex::scoped_lock conf_lock(configuration_mutex_);
    write_enabled_ = true;
  }
  if (write_cb_)
    write_cb_(shared_from_this());
}

void TransportIntraProcess::disableWrite()
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  write_enabled_ = false;
}

void TransportIntraProcess::enableRead()
{
  {
    boost::mutex::scoped_lock conf_lock(configuration_mutex_);
    read_enabled_ = true;
  }
  if (read_cb_)
    read_cb_(shared_from_this());
}

void TransportIntraProcess::disableRead()
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  read_enabled_ = false;
}

void TransportIntraProcess::close()
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  read_enabled_ = false;
  write_enabled_ = false;
}
    
std::string TransportIntraProcess::getTransportInfo()
{
  boost::mutex::scoped_lock(configuration_mutex_);
  std::stringstream str;
  str << "intraprocess connection with " << interlocutor_;
  return str.str();
}

}
