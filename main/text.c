// Text block and time

#include "app.h"
#include "chars.h"

static const char *
showtext (app_t * a, const char *data, uint8_t dokern)
{
   uint8_t flip = 0;
   uint8_t h = a->height;
   if (a->height < 0)
   {
      h = -a->height;
      flip = 1;
   }
   uint8_t w = a->len / h;      // Width
   if (!w)
      return "No space";
   {
      int c = a->stage;         // Whole characters
      while (c && *data)
      {
         c--;
         data++;
         while ((*data & 0xC0) == 0x80)
            data++;             // Unicode
      }
   }
   uint8_t l = 255;
   if (a->limit && a->limit - a->cycle < a->fade)
      l = 255 * (a->limit - a->cycle + 1) / a->fade;
   else if (a->fade && a->cycle < a->fade)
      l = 255 * (a->cycle + 1) / a->fade;
   unsigned char k[8];
   memset (k, 0xFE, 8);
   int c = -(int) a->step;
   while (c < w)
   {
      char t[5],
       *o = t;
      if (*data)
      {
         *o++ = *data++;
         while (o < t + sizeof (t) - 1 && (*data & 0xC0) == 0x80)
            *o++ = *data++;
      }
      *o = 0;
      int i = 0;
      if (*t)
         for (i = 0; i < sizeof (chars) / sizeof (*chars); i++)
            if (!strcmp (chars[i].c, t))
               break;
      if (i == sizeof (chars) / sizeof (*chars))
      {                         // Unknown character
         if (c < 0)
            a->stage++;         // Skip
         continue;
      }
      if (dokern)
      {
         unsigned char k2[8];
         if (!i)
            memset (k2, 0xF8, 8);       // Space
         else
         {
            unsigned char k3[8];
            for (int y = 0; y < 8; y++)
               k3[y] = ((chars[i].b[y]) | (chars[i].b[y] >> 1) | (chars[i].b[y] >> 2));
            k2[0] = (k3[0] | k3[1]);
            for (int y = 1; y < 7; y++)
               k2[y] = (k3[y - 1] | k3[y] | k3[y + 1]);
            k2[7] = (k3[6] | k3[7]);
         }
         for (int x = 6; x > 0; x--)
         {
            int y;
            for (y = 0; y < 8; y++)
               if (((int) k[y] << x) & k2[y])
                  break;
            if (y < 8)
               break;
            c--;
         }
         memcpy (k, k2, 8);
      }
      for (int x = 0; x < 6; x++)
      {
         if (c >= 0 && c < w)
         {
            if ((c ^ flip) & 1)
               for (int y = 0; y < h; y++)
               {
                  if (y < 8 && chars[i].b[y] & (0x80 >> x))
                     setl (a->start + c * h + y, a, l);
            } else
            {
               for (int y = 0; y < h; y++)
                  if (y < 8 && chars[i].b[y] & (0x80 >> x))
                     setl (a->start + c * h + h - 1 - y, a, l);
            }
         }
         c++;
      }
   }
   if (!*data)
   {                            // Back to start
      a->stage = 0;
      a->step = 0;
   } else
   {
      if (++a->step == 6)
      {
         a->stage++;
         a->step = 0;
      }
   }
   return NULL;
}

const char *
apptime (app_t * a)
{
   if (!a->cycle)
   {
      if (!a->colourset)
         a->cycling = 1;
   }
   char temp[6];
   time_t now = time (0);
   struct tm tm;
   localtime_r (&now, &tm);
   snprintf (temp, sizeof (temp), "%02d:%02d", tm.tm_hour, tm.tm_min);
   return showtext (a, temp, 1);
}

const char *
apptext (app_t * a)
{
   if (!a->cycle)
   {
      if (!a->data)
         return "No data";
      if (!a->colourset)
         a->cycling = 1;
   }
   return showtext (a, (const char *) a->data, 0);
}

const char *
appkern (app_t * a)
{
   if (!a->cycle)
   {
      if (!a->data)
         return "No data";
      if (!a->colourset)
         a->cycling = 1;
   }
   return showtext (a, (const char *) a->data, 1);
}
