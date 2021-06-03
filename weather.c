#include <stdlib.h> // For rand()
#include <time.h>
#include <arpa/inet.h>
#include "weather.h"

// A substitute for the function that would really do the work
// Numbers in the range [min, max]
float forecast_for(char *station, float min, float max) {
  srand((unsigned)time(NULL));
   return(min + ((float)rand()) / (((float)RAND_MAX) / (max - min + 1.0) + 1.0));
   // return((float)rand()/(float)RAND_MAX) * 150.0 - 30.0;
}

void hton_range(const struct range *h, struct range *n) {
  n->low  = htons(h->low);
  n->high = htons(h->high);
}

void ntoh_range(const struct range *n, struct range *h) {
  h->low  = ntohs(n->low);
  h->high = ntohs(n->high);
}
