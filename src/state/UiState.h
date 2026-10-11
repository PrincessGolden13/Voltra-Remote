#pragma once
namespace voltra {
struct UiState {
    enum class Screen { Home };
    Screen screen = Screen::Home;
    bool weightSelected = true;
};
}
