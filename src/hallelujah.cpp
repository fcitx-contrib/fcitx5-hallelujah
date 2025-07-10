#include "hallelujah.h"
#include <algorithm>
#include <cctype>
#include <fcitx-utils/standardpaths.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputpanel.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>
#include <marisa/iostream.h>
#include <nlohmann/json.hpp>
#include <spell_public.h>

using json = nlohmann::json;

namespace fcitx::hallelujah {
static const std::array<Key, 10> selectionKeys = {
    Key{FcitxKey_1}, Key{FcitxKey_2}, Key{FcitxKey_3}, Key{FcitxKey_4},
    Key{FcitxKey_5}, Key{FcitxKey_6}, Key{FcitxKey_7}, Key{FcitxKey_8},
    Key{FcitxKey_9}, Key{FcitxKey_0},
};

std::string lower(const std::string &s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](char c) { return std::tolower(c); });
    return r;
}

class HallelujahCandidateWord : public CandidateWord {
public:
    HallelujahCandidateWord(HallelujahState *state, const std::string &word,
                            const std::string &comment)
        : state_(state) {
        setText(Text(word));
        if (!comment.empty()) {
            setComment(Text(comment));
        }
    }

    void select(InputContext *inputContext) const override {
        auto config = state_->engine()->config();
        auto word = text().toString();
        if (*config.commitWithSpace) {
            word += " ";
        }
        inputContext->commitString(word);
        state_->reset(inputContext);
    }

private:
    HallelujahState *state_;
};

class HallelujahCandidateList : public CandidateList,
                                public CursorMovableCandidateList {
public:
    HallelujahCandidateList(HallelujahState *state,
                            const std::vector<std::string> &words,
                            const std::vector<std::string> &comments) {
        setCursorMovable(this);
        for (size_t i = 0; i < words.size(); ++i) {
            labels_.emplace_back(std::to_string((i + 1) % 10) + " ");
            candidateWords_.emplace_back(
                std::make_unique<HallelujahCandidateWord>(state, words[i],
                                                          comments[i]));
        }
    }

    void prevCandidate() override { cursor_ = (cursor_ + size() - 1) % size(); }

    void nextCandidate() override { cursor_ = (cursor_ + 1) % size(); }

    const Text &label(int idx) const override {
        checkIndex(idx);
        return labels_[idx];
    }

    const CandidateWord &candidate(int idx) const override {
        checkIndex(idx);
        return *candidateWords_[idx];
    }

    int size() const override { return candidateWords_.size(); }
    int cursorIndex() const override { return cursor_; }
    CandidateLayoutHint layoutHint() const override {
        return CandidateLayoutHint::Vertical;
    }

private:
    void checkIndex(int idx) const {
        if (idx < 0 || idx >= size()) {
            throw std::invalid_argument("invalid index");
        }
    }

    std::vector<Text> labels_;
    std::vector<std::unique_ptr<CandidateWord>> candidateWords_;
    int cursor_ = 0;
};

void HallelujahState::updatePreedit(InputContext *ic) {
    Text preedit;
    auto userInput = buffer_.userInput();
    if (!userInput.empty()) {
        preedit.append(userInput);
        preedit.setCursor(buffer_.cursor());
    }

    PreeditMode mode = ic->capabilityFlags().test(CapabilityFlag::Preedit)
                           ? *engine_->config().preeditMode
                           : PreeditMode::No;
    auto &inputPanel = ic->inputPanel();
    switch (mode) {
    case PreeditMode::No:
        inputPanel.setPreedit(preedit);
        inputPanel.setClientPreedit(Text());
        break;
    case PreeditMode::ComposingText:
        inputPanel.setClientPreedit(preedit);
        break;
    }

    ic->updatePreedit();
    ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void HallelujahState::updateUI(InputContext *ic,
                               const std::vector<std::string> &words,
                               const std::vector<std::string> &comments) {
    auto &inputPanel = ic->inputPanel();
    if (words.empty()) {
        inputPanel.setCandidateList(nullptr);
    } else {
        inputPanel.setCandidateList(
            std::make_unique<HallelujahCandidateList>(this, words, comments));
    }
    updatePreedit(ic);
}

void HallelujahState::reset(InputContext *ic) {
    buffer_.clear();
    updateUI(ic, {}, {});
}

void HallelujahState::keyEvent(KeyEvent &event) {
    auto key = event.key();
    auto candidateList = ic_->inputPanel().candidateList();
    if (candidateList && candidateList->size()) {
        int idx = key.keyListIndex(selectionKeys);
        if (idx >= 0 && idx < candidateList->size()) {
            candidateList->candidate(idx).select(ic_);
            return event.filterAndAccept();
        }
        if (key.check(FcitxKey_space) || key.check(FcitxKey_Return)) {
            event.filterAndAccept();
            std::string word =
                candidateList->candidate(candidateList->cursorIndex())
                    .text()
                    .toString();
            if (key.check(FcitxKey_space)) {
                word += " ";
            }
            ic_->commitString(word);
            return reset(ic_);
        }
        if (key.check(FcitxKey_Down) || key.check(FcitxKey_Up)) {
            auto cm = candidateList->toCursorMovable();
            if (key.check(FcitxKey_Down)) {
                cm->nextCandidate();
            } else {
                cm->prevCandidate();
            }
            ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
            return event.filterAndAccept();
        }
        if (key.check(FcitxKey_Left) || key.check(FcitxKey_Right) ||
            key.check(FcitxKey_Home) || key.check(FcitxKey_End)) {
            auto size = buffer_.size();
            auto cursor = buffer_.cursor();
            if (key.check(FcitxKey_Home)) {
                cursor = 0;
            } else if (key.check(FcitxKey_End)) {
                cursor = size;
            } else {
                cursor = (cursor + (key.check(FcitxKey_Left) ? size : 1)) %
                         (size + 1);
            }
            buffer_.setCursor(cursor);
            updatePreedit(ic_);
            return event.filterAndAccept();
        }
        if (key.check(FcitxKey_Escape)) {
            reset(ic_);
            return event.filterAndAccept();
        }
    }
    if (key.check(FcitxKey_BackSpace) || key.check(FcitxKey_Delete)) {
        if (buffer_.empty()) {
            return;
        }
        key.check(FcitxKey_BackSpace) ? buffer_.backspace() : buffer_.del();
    } else if (key.isLAZ() || key.isUAZ()) {
        buffer_.type(key.sym());
    } else {
        ic_->commitString(buffer_.userInput());
        return reset(ic_);
    }
    std::vector<std::string> words;
    std::vector<std::string> comments;
    if (!buffer_.empty()) {
        marisa::Agent agent;
        auto userInput = buffer_.userInput();
        auto normalized = lower(userInput);
        agent.set_query(normalized.c_str());
        while (trie_->predictive_search(agent)) {
            words.emplace_back(agent.key().ptr(), agent.key().length());
        }
        auto n = std::min<size_t>(words.size(), 10);
        if (n) {
            std::partial_sort(
                words.begin(), words.begin() + n, words.end(),
                [this, &normalized](const std::string &a,
                                    const std::string &b) {
                    auto na = a == normalized ? 1 : 0;
                    auto nb = b == normalized ? 1 : 0;
                    auto ia = words_->find(a);
                    auto fa = ia == words_->end() ? 0 : ia->second.frequency_;
                    auto ib = words_->find(b);
                    auto fb = ib == words_->end() ? 0 : ib->second.frequency_;
                    return std::tie(na, fa, a) > std::tie(nb, fb, b);
                });
            words.resize(n);
        } else {
            auto iter = pinyin_->find(normalized);
            if (iter != pinyin_->end()) {
                words = iter->second;
            } else {
                words =
                    engine_->spell()->call<ISpell::hint>("en", normalized, 9);
            }
        }
        if (words.empty() || words[0] != normalized) {
            words.emplace(words.begin(), normalized);
        }
        words.resize(std::min<size_t>(words.size(), 10));
        for (auto &word : words) {
            auto iter = words_->find(word);
            if (word.rfind(normalized, 0) != std::string::npos) {
                std::copy(userInput.begin(), userInput.end(), word.begin());
            }
            if (iter != words_->end()) {
                auto config = engine_->config();
                std::string comment = *config.showIPA ? iter->second.ipa_ : "";
                if (!comment.empty()) {
                    comment = fmt::format("[{}] ", comment);
                }
                if (*config.showTranslation) {
                    comment += fmt::format(
                        "{}", fmt::join(iter->second.translation_, " "));
                }
                comments.emplace_back(std::move(comment));
            } else {
                comments.emplace_back("");
            }
        }
    }
    event.filterAndAccept();
    updateUI(ic_, words, comments);
}

HallelujahEngine::HallelujahEngine(Instance *instance)
    : instance_(instance), factory_([this](InputContext &ic) {
          return new HallelujahState(this, &ic, &trie_, &words_, &pinyin_);
      }) {
    loadTrie();
    loadWords();
    loadPinyin();
    instance->inputContextManager().registerProperty("hallelujahState",
                                                     &factory_);
}

HallelujahEngine::~HallelujahEngine() { factory_.unregister(); }

void HallelujahEngine::reset(const InputMethodEntry &,
                             InputContextEvent &event) {
    auto ic = event.inputContext();
    auto state = ic->propertyFor(&factory_);
    state->reset(ic);
}

void HallelujahEngine::loadTrie() {
    const auto &sp = fcitx::StandardPaths::global();
    auto trie_path = sp.locate(fcitx::StandardPathsType::Data,
                               "hallelujah/google_227800_words.bin");
    if (trie_path.empty()) {
        throw std::runtime_error("Failed to locate google_227800_words.bin");
    }
    std::ifstream ifs(trie_path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Failed to open google_227800_words.bin");
    }
    ifs >> trie_;
}

void HallelujahEngine::loadWords() {
    const auto &sp = fcitx::StandardPaths::global();
    auto words_path =
        sp.locate(fcitx::StandardPathsType::Data, "hallelujah/words.json");
    if (words_path.empty()) {
        throw std::runtime_error("Failed to locate words.json");
    }
    std::ifstream ifs(words_path);
    if (!ifs) {
        throw std::runtime_error("Failed to open words.json");
    }
    json obj;
    ifs >> obj;
    if (!obj.is_object()) {
        throw std::runtime_error("Invalid words.json");
    }
    for (auto &[key, value] : obj.items()) {
        if (!value.is_object()) {
            continue;
        }
        const auto &translation = value["translation"];
        const auto &ipa = value["ipa"];
        const auto &frequency = value["frequency"];

        if (!translation.is_array() || !ipa.is_string() ||
            !frequency.is_number_integer()) {
            continue;
        }
        std::vector<std::string> translations;
        for (const auto &item : translation) {
            if (item.is_string()) {
                translations.emplace_back(item.get<std::string>());
            }
        }
        words_.emplace(key, HallelujahWord(std::move(translations),
                                           ipa.get<std::string>(),
                                           frequency.get<int>()));
    }
}

void HallelujahEngine::loadPinyin() {
    const auto &sp = fcitx::StandardPaths::global();
    auto pinyin_path =
        sp.locate(fcitx::StandardPathsType::Data, "hallelujah/cedict.json");
    if (pinyin_path.empty()) {
        throw std::runtime_error("Failed to locate cedict.json");
    }
    std::ifstream ifs(pinyin_path);
    if (!ifs) {
        throw std::runtime_error("Failed to open cedict.json");
    }
    json obj;
    ifs >> obj;
    if (!obj.is_object()) {
        throw std::runtime_error("Invalid cedict.json");
    }

    for (auto &[key, value] : obj.items()) {
        if (!value.is_array()) {
            continue;
        }
        std::vector<std::string> vec;
        for (const auto &item : value) {
            if (item.is_string()) {
                vec.emplace_back(item.get<std::string>());
            }
        }
        pinyin_.emplace(key, std::move(vec));
    }
}

void HallelujahEngine::keyEvent(const InputMethodEntry &, KeyEvent &keyEvent) {
    if (keyEvent.isRelease() || keyEvent.key().states()) {
        return;
    }
    auto ic = keyEvent.inputContext();
    auto *state = ic->propertyFor(&factory_);
    state->keyEvent(keyEvent);
}

void HallelujahEngine::reloadConfig() { readAsIni(config_, ConfPath); }

void HallelujahEngine::setConfig(const RawConfig &config) {
    config_.load(config, true);
    safeSaveAsIni(config_, ConfPath);
    reloadConfig();
}
} // namespace fcitx::hallelujah
