#include "testdir.h"
#include "testfrontend_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/instance.h>

using namespace fcitx;

std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>>
    expectedCommit{
        {{"a", "4"}, {"are"}},
        {{"a", "space"}, {"a "}},
        {{"a", "Return"}, {"a"}},
        {{"a", "Down", "space"}, {"at "}},
        {{"a", "Up", "Return"}, {"also"}},
        {{"a", "Escape"}, {}},
        {{"a", "b", "BackSpace", "Return"}, {"a"}},
        {{"a", "X", "e", "Return"}, {"aXe"}},
        {{"a", ","}, {"a"}}, // comma is passed to client
        {{"s", "h", "u", "r", "u", "f", "a", "3"}, {"input method"}},
    };

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([dispatcher, instance]() {
        auto *hallelujah = instance->addonManager().addon("hallelujah", true);
        FCITX_ASSERT(hallelujah);
        auto defaultGroup = instance->inputMethodManager().currentGroup();
        defaultGroup.inputMethodList().clear();
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("keyboard-us"));
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("hallelujah"));
        defaultGroup.setDefaultInputMethod("");
        instance->inputMethodManager().setGroup(defaultGroup);
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+space"),
                                                    false);
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
    setupTestingEnvironment(
        TESTING_BINARY_DIR, {TESTING_BINARY_DIR "/src"},
        {TESTING_BINARY_DIR "/test", StandardPath::fcitxPath("pkgdatadir")});
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
