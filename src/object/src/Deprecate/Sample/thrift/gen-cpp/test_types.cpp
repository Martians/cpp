/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "test_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TTocstring>

namespace test {


WorkTask::~WorkTask() throw() {
}


void WorkTask::__set_V32(const int32_t val) {
  this->V32 = val;
}

void WorkTask::__set_V64(const int64_t val) {
  this->V64 = val;
}

void WorkTask::__set_Vstr(const std::string& val) {
  this->Vstr = val;
}

uint32_t WorkTask::read(::apache::thrift::protocol::TProtocol* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->V32);
          this->__isset.V32 = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->V64);
          this->__isset.V64 = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->Vstr);
          this->__isset.Vstr = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t WorkTask::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("WorkTask");

  xfer += oprot->writeFieldBegin("V32", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->V32);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("V64", ::apache::thrift::protocol::T_I64, 2);
  xfer += oprot->writeI64(this->V64);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("Vstr", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString(this->Vstr);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(WorkTask &a, WorkTask &b) {
  using ::std::swap;
  swap(a.V32, b.V32);
  swap(a.V64, b.V64);
  swap(a.Vstr, b.Vstr);
  swap(a.__isset, b.__isset);
}

WorkTask::WorkTask(const WorkTask& other0) {
  V32 = other0.V32;
  V64 = other0.V64;
  Vstr = other0.Vstr;
  __isset = other0.__isset;
}
WorkTask& WorkTask::operator=(const WorkTask& other1) {
  V32 = other1.V32;
  V64 = other1.V64;
  Vstr = other1.Vstr;
  __isset = other1.__isset;
  return *this;
}
void WorkTask::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "WorkTask(";
  out << "V32=" << to_string(V32);
  out << ", " << "V64=" << to_string(V64);
  out << ", " << "Vstr=" << to_string(Vstr);
  out << ")";
}

} // namespace