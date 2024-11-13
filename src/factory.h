#ifndef _FCITX5_HALLELUJAH_FACTORY_H_
#define _FCITX5_HALLELUJAH_FACTORY_H_

#include <fcitx/addonfactory.h>

namespace fcitx {
class HallelujahFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override;
};
} // namespace fcitx
#endif
