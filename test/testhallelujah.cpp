#include "testfrontend_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/log.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/instance.h>

using namespace fcitx;

std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>>
    expectedCommit{
        {{"a", "4"}, {"at"}}, // a: exact match; and are at: frequency order.
        {{"a", "space"}, {"a "}},
        {{"a", "Return"}, {"a"}},
        {{"a", "Down", "space"}, {"and "}},
        {{"a", "Up", "Return"}, {"also"}},
        {{"a", "Right", "b", "Left", "Left", "c", "Return"}, {"bac"}},
        {{"a", "Escape"}, {}},
        {{"a", "b", "BackSpace", "Return"}, {"a"}},
        {{"a", "b", "c", "Home", "Delete", "End", "d", "Return"}, {"bcd"}},
        {{"a", "X", "e", "Return"}, {"aXe"}},
        {{"a", ","}, {"a"}}, // comma is passed to client
        {{"s", "h", "u", "r", "u", "f", "a", "3"}, {"input method"}},
        {{"e", "x", "c", "i", "t", "n", "g", "2"}, {"exciting"}}, // spell check
    };

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([dispatcher, instance]() {
        auto *hallelujah = instance->addonManager().addon("hallelujah", true);
        FCITX_ASSERT(hallelujah);
        auto defaultGroup = instance->inputMethodManager().currentGroup();
        defaultGroup.inputMethodList().clear();
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("hallelujah"));
        defaultGroup.setDefaultInputMethod("");
        instance->inputMethodManager().setGroup(defaultGroup);
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        for (const auto &data : expectedCommit) {
            for (const auto &expect : data.second) {
                testfrontend->call<ITestFrontend::pushCommitExpectation>(
                    expect);
            }
            for (const auto &key : data.first) {
                testfrontend->call<ITestFrontend::keyEvent>(uuid, Key(key),
                                                            false);
            }
        }
        instance->deactivate();
        dispatcher->schedule([dispatcher, instance]() {
            dispatcher->detach();
            instance->exit();
        });
    });
}

int main() {
    char arg0[] = "testhallelujah";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,spell,hallelujah";
    char *argv[] = {arg0, arg1, arg2};
    fcitx::Log::setLogRule("default=5,hallelujah=5");
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    scheduleEvent(&dispatcher, &instance);
    instance.exec();
    return 0;
}
