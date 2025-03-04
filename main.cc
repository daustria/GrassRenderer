#include "scenemanager.h"
#include "windowmanager.h"
#include <cstdio>
#include <iostream>

int main()
{
    WindowManager* manager = new SceneManager();
    manager->start_window();
    return 0;
}