#ifndef _FCITX5_HALLELUJAH_HALLELUJAH_H_
#define _FCITX5_HALLELUJAH_HALLELUJAH_H_

#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <marisa/trie.h>
#include <unordered_map>

namespace fcitx::hallelujah {
enum class PreeditMode { No, ComposingText };

FCITX_CONFIG_ENUM_NAME_WITH_I18N(PreeditMode, N_("Do not show"),
                                 N_("Composing text"))

FCITX_CONFIGURATION(
    HallelujahEngineConfig,
    OptionWithAnnotation<PreeditMode, PreeditModeI18NAnnotation> preeditMode{
        this, "PreeditMode", _("Preedit Mode"), PreeditMode::ComposingText};
    Option<bool> showIPA{this, "ShowIPA", _("Show IPA"), true};
    Option<bool> showTranslation{this, "ShowTranslation", _("Show translation"),
                                 true};
    Option<bool> commitWithSpace{this, "CommitWithSpace",
                                 _("Commit with space"), false};);

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
    void updateUI(InputContext *ic, const std::vector<std::string> &words,
                  const std::vector<std::string> &candidates);
    void reset(InputContext *ic);
    HallelujahEngine *engine() { return engine_; }

private:
    void updatePreedit(InputContext *ic);
    HallelujahEngine *engine_;
    InputContext *ic_;
    InputBuffer buffer_{{InputBufferOption::AsciiOnly}};
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
    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override;
    void reloadConfig() override;
    const HallelujahEngineConfig &config() const { return config_; }
    FCITX_ADDON_DEPENDENCY_LOADER(spell, instance_->addonManager());

private:
    void loadWords();
    void loadTrie();
    void loadPinyin();

    Instance *instance_;
    HallelujahEngineConfig config_;
    FactoryFor<HallelujahState> factory_;
    marisa::Trie trie_;
    std::unordered_map<std::string, HallelujahWord> words_;
    std::unordered_map<std::string, std::vector<std::string>> pinyin_;
    static const inline std::string ConfPath = "conf/hallelujah.conf";
};
} // namespace fcitx::hallelujah

#endif
