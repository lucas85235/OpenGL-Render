#include "src/core/application.hpp"

int main() {
  Application app("OpenGL Render", 1280, 720);

  if (!app.Initialize()) {
    return -1;
  }

  while (!app.ShouldClose()) {
    app.Tick();
  }

  return 0;
}