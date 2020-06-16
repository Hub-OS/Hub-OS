#include "bnGame.h"
#include "bnQueueNaviRegistration.h"
#include "bnQueueMobRegistration.h"

int main(int argc, char** argv) {
  QueuNaviRegistration(); // Queues navis to be loaded later
  QueueMobRegistration(); // Queues mobs to be loaded later

  Game game;
  game.Boot();

  // blocking
  game.Poll();

  // finished
  return EXIT_SUCCESS;
}