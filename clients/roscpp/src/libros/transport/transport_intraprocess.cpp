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
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  boost::mutex::scoped_lock buf_lock(buffer_mutex_);
  if (!read_enabled_)
    return 0;
  std::copy(buffer, buffer + size, std::back_inserter(buffer_));
  return size;
}

uint32_t read(uint8_t* buffer, uint32_t size)
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  if (!read_enabled_)
    return 0;
  
  boost::mutex::scoped_lock buf_lock(buffer_mutex_);
  size = std::min(size, (uint32_t)buffer_.size());
  std::deque<uint8_t> end = buffer_.begin() + size;
  for (std::deque<uint8_t>::iterator it = buffer_.beginI(); it != end; it++)
    *(buffer++) = *it;
  return size;
}

uint32_t write(uint8_t* buffer, uint32_t size)
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  if (!write_enabled_)
    return 0;

  // interlocutor should not be NULL if write is enabled
  ROS_ASSERT(interlocutor != NULL);
  return interlocutor->accept(buffer, size);
}

void enableWrite()
{
  {
    boost::mutex::scoped_lock conf_lock(configuration_mutex_);
    write_enabled_ = true;
  }
  if (write_cb_)
    write_cb_();
}

void disableWrite()
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  write_enabled_ = false;
}

void enableRead()
{
  {
    boost::mutex::scoped_lock conf_lock(configuration_mutex_);
    read_enabled_ = true;
  }
  if (read_cb_)
    read_cb_();
}

void disableRead()
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  read_enabled_ = false;
}

void close()
{
  boost::mutex::scoped_lock conf_lock(configuration_mutex_);
  read_enabled_ = false;
  write_enabled_ = false;
}

std::string getTransportInfo()
{
  boost::mutex::scoped_lock(configuration_mutex_);
  std::stringstream str;
  str << "intraprocess connection with " << interlocutor_;
  return str.str();
}


