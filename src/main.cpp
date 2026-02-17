#include "core/Game.h"

int main()
{
    Game game;
    game.Init();

    while (!game.ShouldClose())
    {
        game.Update();
        game.Draw();
    }

    game.Shutdown();
    return 0;
}
