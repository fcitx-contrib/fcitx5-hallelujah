#include "factory.h"
#include "hallelujah.h"

namespace fcitx::hallelujah {
AddonInstance *HallelujahFactory::create(AddonManager *manager) {
    registerDomain("fcitx5-hallelujah", FCITX_INSTALL_LOCALEDIR);
    return new HallelujahEngine(manager->instance());
}
} // namespace fcitx::hallelujah

FCITX_ADDON_FACTORY_V2(hallelujah, fcitx::hallelujah::HallelujahFactory)
