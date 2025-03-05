#include <stdio.h>
#include "space_invaders.h"

int main(void) {
  printf("Space Invaders!\n");

  SpaceInvaders *si = new();
  run(si);
}
