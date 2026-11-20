#include "core/plugin.h"
#include "module_transit_demod.h"
#include "module_transit_decoder.h"
#include "transit_product_handler.h"

class TransitSupport : public satdump::Plugin
{
public:
    std::string getID() { return "transit_support"; }
    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerModules);
        satdump::eventBus->register_handler<satdump::handlers::RequestProductHandlerEvent>(
            [](const satdump::handlers::RequestProductHandlerEvent &evt)
            {
                if (evt.product->type == "transit_telemetry")
                    evt.handler = std::make_shared<transit::TransitProductHandler>(evt.product, evt.dataset_mode);
            });
    }
    static void registerModules(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, transit::TransitDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, transit::TransitDecoderModule);
    }
};
PLUGIN_LOADER(TransitSupport)
