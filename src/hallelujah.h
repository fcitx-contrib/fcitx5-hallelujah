#ifndef _FCITX5_SAYURA_SAYURA_H_
#define _FCITX5_SAYURA_SAYURA_H_

#include <fcitx-utils/i18n.h>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <marisa/trie.h>
#include <unordered_map>

namespace fcitx {
struct HallelujahWord {
    HallelujahWord(const std::vector<std::string> &translation,
                   const std::string &ipa, int frequency)
        : translation_(translation), ipa_(ipa), frequency_(frequency) {}
    std::vector<std::string> translation_;
    std::string ipa_;
    int frequency_;
};

class HallelujahEngine;

class HallelujahState : public InputContextProperty {
public:
    HallelujahState(
        HallelujahEngine *engine, InputContext *ic, marisa::Trie *trie,
        std::unordered_map<std::string, HallelujahWord> *words,
        std::unordered_map<std::string, std::vector<std::string>> *pinyin)
        : engine_(engine), ic_(ic), trie_(trie), words_(words),
          pinyin_(pinyin) {}
    void keyEvent(KeyEvent &keyEvent);
    void updateUI(InputContext *ic, const std::vector<std::string> words,
                  const std::vector<std::string> candidates);
    void reset(InputContext *ic);

private:
    HallelujahEngine *engine_;
    InputContext *ic_;
    InputBuffer buffer_{
        {InputBufferOption::AsciiOnly, InputBufferOption::FixedCursor}};
    marisa::Trie *trie_;
    std::unordered_map<std::string, HallelujahWord> *words_;
    std::unordered_map<std::string, std::vector<std::string>> *pinyin_;
};

class HallelujahEngine final : public InputMethodEngine {
public:
    HallelujahEngine(Instance *instance);
    ~HallelujahEngine();

    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    void reset(const InputMethodEntry &, InputContextEvent &event) override;
    FCITX_ADDON_DEPENDENCY_LOADER(spell, instance_->addonManager());

private:
    void loadWords();
    void loadTrie();
    void loadPinyin();

    Instance *instance_;
    FactoryFor<HallelujahState> factory_;
    marisa::Trie trie_;
    std::unordered_map<std::string, HallelujahWord> words_;
    std::unordered_map<std::string, std::vector<std::string>> pinyin_;
};

class HallelujahFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        registerDomain("fcitx5-hallelujah", FCITX_INSTALL_LOCALEDIR);
        return new HallelujahEngine(manager->instance());
    }
};
} // namespace fcitx

#endif
