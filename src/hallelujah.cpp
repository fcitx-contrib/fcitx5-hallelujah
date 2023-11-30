#include "hallelujah.h"
#include <algorithm>
#include <cctype>
#include <fcitx-utils/standardpath.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputpanel.h>
#include <fmt/format.h>
#include <json-c/json.h>
#include <spell_public.h>

namespace fcitx {
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
    HallelujahCandidateWord(HallelujahState *state, std::string word,
                            std::string candidate)
        : state_(state), word_(word) {
        setText(Text(candidate));
    }

    void select(InputContext *inputContext) const override {
        inputContext->commitString(word_);
        state_->reset(inputContext);
    }

    std::string getWord() const { return word_; }

private:
    HallelujahState *state_;
    std::string word_;
};

class HallelujahCandidateList : public CandidateList,
                                public CursorMovableCandidateList {
public:
    HallelujahCandidateList(HallelujahState *state,
                            const std::vector<std::string> words,
                            const std::vector<std::string> candidates) {
        setCursorMovable(this);
        for (size_t i = 0; i < words.size(); ++i) {
            labels_.emplace_back(std::to_string((i + 1) % 10) + " ");
            candidateWords_.emplace_back(
                std::make_unique<HallelujahCandidateWord>(state, words[i],
                                                          candidates[i]));
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

void HallelujahState::updateUI(InputContext *ic,
                               const std::vector<std::string> words,
                               const std::vector<std::string> candidates) {
    auto &inputPanel = ic->inputPanel();
    if (words.empty()) {
        inputPanel.setCandidateList(nullptr);
    } else {
        inputPanel.setCandidateList(
            std::make_unique<HallelujahCandidateList>(this, words, candidates));
    }
    Text preedit;
    preedit.append(buffer_.userInput());
    inputPanel.setPreedit(preedit);
    ic->updateUserInterface(UserInterfaceComponent::InputPanel);
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
                dynamic_cast<const HallelujahCandidateWord &>(
                    candidateList->candidate(candidateList->cursorIndex()))
                    .getWord();
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
        if (key.check(FcitxKey_Escape)) {
            reset(ic_);
            return event.filterAndAccept();
        }
    }
    if (key.check(FcitxKey_BackSpace)) {
        if (buffer_.empty()) {
            return;
        }
        buffer_.backspace();
    } else if (key.isLAZ() || key.isUAZ()) {
        buffer_.type(key.sym());
    } else {
        ic_->commitString(buffer_.userInput());
        return reset(ic_);
    }
    std::vector<std::string> words;
    std::vector<std::string> candidates;
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
                    if (a == normalized) {
                        return true;
                    }
                    if (b == normalized) {
                        return false;
                    }
                    auto ia = words_->find(a);
                    auto fa = ia == words_->end() ? 0 : ia->second.frequency_;
                    auto ib = words_->find(b);
                    auto fb = ib == words_->end() ? 0 : ib->second.frequency_;
                    return fa > fb;
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
                auto ipa = iter->second.ipa_;
                if (!ipa.empty()) {
                    ipa = fmt::format("[{}] ", ipa);
                }
                candidates.emplace_back(
                    fmt::format("{} {}{}", word, ipa,
                                fmt::join(iter->second.translation_, " ")));
            } else {
                candidates.emplace_back(word);
            }
        }
    }
    event.filterAndAccept();
    updateUI(ic_, words, candidates);
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

void HallelujahEngine::activate(const InputMethodEntry &, InputContextEvent &) {
    spell();
}

void HallelujahEngine::reset(const InputMethodEntry &,
                             InputContextEvent &event) {
    auto ic = event.inputContext();
    auto state = ic->propertyFor(&factory_);
    state->reset(ic);
}

void HallelujahEngine::loadTrie() {
    const auto &sp = fcitx::StandardPath::global();
    std::string trie_path = sp.locate(fcitx::StandardPath::Type::Data,
                                      "hallelujah/google_227800_words.bin");
    if (trie_path.empty()) {
        throw std::runtime_error("Failed to load google_227800_words.bin");
    }
    trie_.load(trie_path.c_str());
}

void HallelujahEngine::loadWords() {
    const auto &sp = fcitx::StandardPath::global();
    std::string words_path =
        sp.locate(fcitx::StandardPath::Type::Data, "hallelujah/words.json");
    if (words_path.empty()) {
        throw std::runtime_error("Failed to load words.json");
    }
    UniqueCPtr<json_object, json_object_put> obj;
    obj.reset(json_object_from_file(words_path.c_str()));
    if (!obj || json_object_get_type(obj.get()) != json_type_object) {
        throw std::runtime_error("Invalid words.json");
    }
    json_object_object_foreach(obj.get(), key, value) {
        if (json_object_get_type(value) != json_type_object) {
            continue;
        }
        auto translation = json_object_object_get(value, "translation");
        auto ipa = json_object_object_get(value, "ipa");
        auto frequency = json_object_object_get(value, "frequency");
        if (json_object_get_type(translation) != json_type_array ||
            json_object_get_type(ipa) != json_type_string ||
            json_object_get_type(frequency) != json_type_int) {
            continue;
        }
        std::vector<std::string> translations;
        auto n = json_object_array_length(translation);
        for (size_t i = 0; i < n; ++i) {
            auto item = json_object_array_get_idx(translation, i);
            if (json_object_get_type(item) != json_type_string) {
                continue;
            }
            translations.emplace_back(json_object_get_string(item));
        }
        words_.emplace(key, HallelujahWord(std::move(translations),
                                           json_object_get_string(ipa),
                                           json_object_get_int(frequency)));
    }
}

void HallelujahEngine::loadPinyin() {
    const auto &sp = fcitx::StandardPath::global();
    std::string pinyin_path =
        sp.locate(fcitx::StandardPath::Type::Data, "hallelujah/cedict.json");
    if (pinyin_path.empty()) {
        throw std::runtime_error("Failed to load cedict.json");
    }
    UniqueCPtr<json_object, json_object_put> obj;
    obj.reset(json_object_from_file(pinyin_path.c_str()));
    if (!obj || json_object_get_type(obj.get()) != json_type_object) {
        throw std::runtime_error("Invalid cedict.json");
    }
    json_object_object_foreach(obj.get(), key, value) {
        if (json_object_get_type(value) != json_type_array) {
            continue;
        }
        std::vector<std::string> vec;
        auto n = json_object_array_length(value);
        for (size_t i = 0; i < n; ++i) {
            auto item = json_object_array_get_idx(value, i);
            if (json_object_get_type(item) != json_type_string) {
                continue;
            }
            vec.emplace_back(json_object_get_string(item));
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
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::HallelujahFactory);
