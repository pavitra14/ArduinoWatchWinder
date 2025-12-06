#include "WatchWinderApp.h"

#ifndef UNIT_TEST
WatchWinderApp app;

void setup() {
  app.begin();
}

void loop() {
  app.tick();
}
#endif
