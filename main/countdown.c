// Countdown

#include "app.h"

const char *
appcountdown (app_t * a)
{
   if (!a->cycle)
   {                            // Sanity check / defaults
      if (!a->colourset)
         a->cycling = 1;
      if (!a->limit)
         a->limit = 60 * cps;
   }
   if (a->cycle >= a->limit)
      return "";                // done straight away
   uint8_t l = 255;
   if (a->stop)
      l = 255 * a->stop / a->fade;
   else if (a->fade && a->cycle < a->fade)
      l = 255 * a->cycle / a->fade;
   uint32_t n = (uint32_t) 256 * a->len * (a->limit - a->cycle) / a->limit;
   uint8_t f = n & 255;
   n /= 256;
   for (unsigned int i = 0; i < n; i++)
      setl (a->start + i, a, l);
   if (!a->stop)
      setl (a->start + n, a, (int)l*f/255);
   return NULL;
}
