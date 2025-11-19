
#include "def.h"


LPNRange::_LPNRange() : slpn(0), nlp(0) {}

LPNRange::_LPNRange(uint64_t s, uint64_t n) : slpn(s), nlp(n) {}

namespace HIL {

Request::_Request()
    : reqID(0),
      reqSubID(0),
      offset(0),
      length(0),
      finishedAt(0),
      context(nullptr) {}

Request::_Request(DMAFunction &f, void *c)
    : reqID(0),
      reqSubID(0),
      offset(0),
      length(0),
      finishedAt(0),
      function(f),
      context(c) {}

bool Request::operator()(const Request &a, const Request &b) {
  return a.finishedAt > b.finishedAt;
}

}  // namespace HIL

namespace ICL {

Request::_Request() : reqID(0), reqSubID(0), offset(0), length(0) {}

Request::_Request(HIL::Request &r)
    : reqID(r.reqID),
      reqSubID(r.reqSubID),
      offset(r.offset),
      length(r.length),
      range(r.range) {}

}  // namespace ICL

namespace FTL {

Request::_Request(uint32_t iocount)
    : reqID(0), reqSubID(0), lpn(0), ioFlag(iocount) {}

Request::_Request(uint32_t iocount, ICL::Request &r)
    : reqID(r.reqID),
      reqSubID(r.reqSubID),
      lpn(r.range.slpn / iocount),
      ioFlag(iocount) {
  ioFlag.set(r.range.slpn % iocount);
}

}  // namespace FTL

namespace PAL {

Request::_Request(uint32_t iocount)
    : reqID(0), reqSubID(0), blockIndex(0), pageIndex(0), ioFlag(iocount) {}

Request::_Request(FTL::Request &r)
    : reqID(r.reqID),
      reqSubID(r.reqSubID),
      blockIndex(0),
      pageIndex(0),
      ioFlag(r.ioFlag) {}

}  // namespace PAL

