#pragma once

#include <SDL3/SDL.h>

class EL9300;

// The dot matrix display of the calculator.
class LcdView {
public:
    explicit LcdView(SDL_Renderer *renderer);
    ~LcdView();

    void draw(const EL9300 &emu);

private:
    SDL_Texture *texture = nullptr;
};
