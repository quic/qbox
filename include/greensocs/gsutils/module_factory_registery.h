
/*
 *  Copyright (C) 2023  Qualcomm
 *
 */

#ifndef GREENSOCS_MODULE_FACTORY_REG_H
#define GREENSOCS_MODULE_FACTORY_REG_H

#define CCI_GS_MF_NAME "__GS.ModuleFactory."

#include <greensocs/gsutils/cciutils.h>

/**
 * @brief Helper macro to register an sc_module constructor, complete with its (typed) arguments
 *
 */

namespace gs {
namespace ModuleFactory {
std::vector<std::function<cci::cci_param<gs::cci_constructor_vl>*()>>* GetAvailableModuleList();

struct ModuleRegistrationWrapper {
    ModuleRegistrationWrapper(std::function<cci::cci_param<gs::cci_constructor_vl>*()> fn)
    {
        auto mods = gs::ModuleFactory::GetAvailableModuleList();
        mods->push_back(fn);
    }
};
} // namespace ModuleFactory
} // namespace gs
/**
 * @brief CCI value converted
 * notice that NO conversion is provided.
 *
 */
template <>
struct cci::cci_value_converter<gs::cci_constructor_vl> {
    typedef gs::cci_constructor_vl type;
    static bool pack(cci::cci_value::reference dst, type const& src)
    {
        dst.set_string(src.type);
        return true;
    }
    static bool unpack(type& dst, cci::cci_value::const_reference src)
    {
        if (!src.is_string()) {
            return false;
        }
        std::string moduletype;
        if (!src.try_get(moduletype)) {
            return false;
        }
        cci::cci_param_typed_handle<gs::cci_constructor_vl> m_fac(
            cci::cci_get_broker().get_param_handle(CCI_GS_MF_NAME + moduletype));
        if (!m_fac.is_valid()) {
            SC_REPORT_ERROR("ModuleFactory", ("Can't find module type: " + moduletype).c_str());
            return false;
        }
        dst = *m_fac;

        return true;
    }
};
#define GSC_MODULE_REGISTER(__NAME__, ...)                                                                             \
    struct GS_MODULEFACTORY_moduleReg_##__NAME__ {                                                                     \
        static gs::ModuleFactory::ModuleRegistrationWrapper& get()                                                     \
        {                                                                                                              \
            static gs::ModuleFactory::ModuleRegistrationWrapper inst([]() -> cci::cci_param<gs::cci_constructor_vl>* { \
                return new cci::cci_param<gs::cci_constructor_vl>(                                                     \
                    CCI_GS_MF_NAME #__NAME__,                                                                          \
                    gs::cci_constructor_vl::FactoryMaker<__NAME__, ##__VA_ARGS__>(#__NAME__), "default constructor",   \
                    cci::CCI_ABSOLUTE_NAME);                                                                           \
            });                                                                                                        \
            return inst;                                                                                               \
        }                                                                                                              \
    };                                                                                                                 \
    static struct gs::ModuleFactory::ModuleRegistrationWrapper&                                                        \
        GS_MODULEFACTORY_moduleReg_inst_##__NAME__ = GS_MODULEFACTORY_moduleReg_##__NAME__::get()

#endif
