#pragma once

#include <string>

class App;
class EL9300;

class DebuggerView {
public:
    void draw(App &app);

private:
    std::string buildStateText(EL9300 &emu, bool running);
};
