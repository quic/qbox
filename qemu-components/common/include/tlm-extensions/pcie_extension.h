#ifndef _GREENSOCS_PCIE_EXTENSION_H
#define _GREENSOCS_PCIE_EXTENSION_H

#include <tlm>

namespace gs {

class PcieExtension : public tlm::tlm_extension<PcieExtension>
{
public:
    uint16_t requester_id;
    uint8_t tag;
    
    PcieExtension() : requester_id(0), tag(0) {}
    
    virtual tlm_extension_base* clone() const override {
        PcieExtension* ext = new PcieExtension();
        ext->requester_id = this->requester_id;
        ext->tag = this->tag;
        return ext;
    }
    
    virtual void copy_from(tlm_extension_base const& ext) override {
        requester_id = static_cast<PcieExtension const&>(ext).requester_id;
        tag = static_cast<PcieExtension const&>(ext).tag;
    }
};

} // namespace gs

#endif