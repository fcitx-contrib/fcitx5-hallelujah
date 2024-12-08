#include "factory.h"
#include "hallelujah.h"

namespace fcitx {
AddonInstance *HallelujahFactory::create(AddonManager *manager) {
    return new HallelujahEngine(manager->instance());
}
} // namespace fcitx

FCITX_ADDON_FACTORY_V2(hallelujah, fcitx::HallelujahFactory)
