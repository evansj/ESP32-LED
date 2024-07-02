// Stargate

#include "app.h"
#define SPEED 2                 // Seconds per spin?

const char *
apptwinkle (app_t * a)
{
   if (!a->cycle)
   {
      free (a->data);           // Not used supplied
      a->data = malloc (a->len * 2);
      memset (a->data, 0, a->len * 2);
      if (!a->colourset)
         a->r = a->g = a->b = 255;      // default white
      a->step = a->speed;
   }
   uint8_t *old = a->data,
      *new = old + a->len;
   uint8_t q = a->fader;

   if (!--a->step)
   {                            // Next
      a->step = a->speed;
      memcpy (old, new, a->len);
      esp_fill_random (new, a->len);
      for (int i = 0; i < a->len; i++)
         new[i] = new[i] / 4 + 32;
   }

   for (int i = 0; i < a->len; i++)
   {
      uint8_t l = (int) (a->speed - a->step) * new[i] / a->speed + (int) a->step * old[i] / a->speed;
      setl (a->start + i, a, l * q / 255);
   }

   return NULL;
}
