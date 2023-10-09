// Stargate

#include "app.h"
#define SPEED 2                 // Seconds per spin?

typedef struct ring_s ring_t;
struct ring_s
{                               // Definition of a ring
   uint16_t len;
   uint16_t start;
   uint16_t offset;
};

typedef struct stargate_s stargate_t;
struct stargate_s
{
   uint8_t dial[10];            // Dial sequence 1-39 terminated with 0
   uint8_t pos;                 // Symbol at the top
   uint8_t spins;               // The spinning rings
   const ring_t *spin;
   uint8_t chevs;               // The chevrons
   const ring_t *chev;
   uint8_t gates;               // The gate symbols
   const ring_t *gate;
};

const ring_t spinsmall[] = { {117, 1, 0} };
const ring_t spinbig[] = { {117, 1, 58}, {117, 175, 58} };
const ring_t chevsmall[] = { {117, 1, 0}, {18, 157, 0}, {18, 175, 0}, {18, 193, 0} };
const ring_t chevbig[] = { {18, 157, 8}, {117, 175, 58}, {18, 292, 8}, {18, 310, 8}, {45, 328, 20} };
const ring_t gatesmall[] = { {39, 118, 0} };
const ring_t gatebig[] = { {39, 118, 18} };

const char *
biggate (app_t * a)
{                               // Special large LED rings
   uint8_t max = 127;           // Base brightmess
   uint8_t spinlen = (a->len == 210 ? spinsmall[0].len : spinbig[0].len),
      *old = a->data,
      *new = old + spinlen;
   stargate_t *g = (void *) (new + spinlen);
   if (!a->cycle)
   {                            // Startup
      if (!a->limit)
         a->limit = 90 * cps;
      old = malloc (spinlen * 2 + sizeof (stargate_t));
      new = old + spinlen;
      g = (void *) (new + spinlen);
      memset (g, 0, sizeof (*g));
      if (a->len == 210)
      {
         g->spin = spinsmall;
         g->spins = sizeof (spinsmall) / sizeof (*spinsmall);
         g->chev = chevsmall;
         g->chevs = sizeof (chevsmall) / sizeof (*chevsmall);
         g->gate = gatesmall;
         g->gates = sizeof (gatesmall) / sizeof (*gatesmall);
      } else
      {
         g->spin = spinbig;
         g->spins = sizeof (spinbig) / sizeof (*spinbig);
         g->chev = chevbig;
         g->chevs = sizeof (chevbig) / sizeof (*chevbig);
         g->gate = gatebig;
         g->gates = sizeof (gatebig) / sizeof (*gatebig);
      }
      g->pos = 1;               // Home symbol
      if (a->data)
      {                         // Dial sequence specified
         const char glyphs[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ&-0123456789";
         for (int q = 0; q < 9 && ((char *) a->data)[q]; q++)
         {
            const char *z = strchr (glyphs, ((char *) a->data)[q]);
            if (!z)
               return "Bad dial string";
            g->dial[q] = 1 + (z - glyphs);
         }
         free (a->data);
      } else
      {                         // Random dial sequence
         int n = 6;
         if (esp_random () >= 0xF0000000)
            n = 8;
         else if (esp_random () >= 0xE0000000)
            n = 7;
         uint64_t f = 0;
         for (int q = 0; q < n; q++)
         {
            int r = esp_random () % (38 - q) + 1,
               p = 1;
            while (r || (f & (1LL << p)))
               if ((f & (1LL << p)) || r--)
                  p++;
            f |= (1LL << p);
            g->dial[q] = p;
         }
         g->dial[n] = 1;        // Final
      }
      a->data = old;
   }
   uint8_t q = 255;
   if (a->stop)
      q = 255 * a->stop / a->fade;      // Main fader for end

   void twinkle (void)
   {
      memcpy (old, new, spinlen);
      esp_fill_random (new, spinlen);
      for (int i = 0; i < spinlen; i++)
         new[i] = new[i] / 3 + 32;
   }
   void spinner (uint8_t o)
   {                            // Show 1 in 3 on spin rings
      for (int s = 0; s < g->spins; s++)
         for (int n = 0; n < g->spin[s].len; n++)
            setrgbl (a->start - 1 + g->spin[s].start + (n + o + g->spin[s].offset) % g->spin[s].len, 0, 0, n % 3 ? 0 : max, q);
   }
   void chev (uint8_t c, uint8_t f, uint8_t t, uint8_t l)
   {
      const uint8_t map[] = { 1, 8, 2, 7, 3, 6, 4, 5, 0 };
      if (c < 8 && !g->dial[c + 1])
         c = 0;
      else
         c = map[c];
      for (int n = f; n <= t; n++)
      {
         const ring_t *C = &g->chev[n];
         if (C->len == 117)
         {                      // Chevs part of full ring
            setrgbl (a->start - 1 + C->start + (c * 13 + 116 + C->offset) % C->len, max, max, 0, l);
            setrgbl (a->start - 1 + C->start + (c * 13 + 1 + C->offset) % C->len, max, max, 0, l);
         } else
         {                      // Chevs only
            uint8_t t = C->len / 9;
            uint16_t b = c * t;
            for (int q = 0; q < t; q++)
               setrgbl (a->start - 1 + C->start + (b + q + C->offset) % C->len, max, max, 0, l);
         }
      }
   }
   void gates (uint8_t c, uint8_t l)
   {
      if (g->dial[c])
         for (int z = 0; z < g->gates; z++)
         {
            if (g->gate[z].len == 39)
               setrgbl (a->start - 1 + g->gate[z].start + (g->dial[c] - 1) % g->gate[z].len, max, 0, 0, l);
         }
   }
   void chevs (void)
   {
      for (int c = 0; c < a->stage / 10 - 1; c++)
      {
         chev (c, 0, g->chevs - 1, q);
         gates (c, q);
      }
   }

   if (a->stage < 10)
      switch (a->stage)
      {
      case 0:                  // Fade up spins
         for (int s = 0; s < g->spins; s++)
            for (int n = 0; n < g->spin[s].len; n++)
               setrgbl (a->start - 1 + g->spin[s].start + n, 0, 0, max, a->step * q / 255);
         if ((a->step += 255 / a->speed) > 255)
         {
            a->step = 0;
            a->stage++;
         }
         break;
      case 1:                  // Fade down spins leaving 1/3 to dial
         for (int s = 0; s < g->spins; s++)
            for (int n = 0; n < g->spin[s].len; n++)
               setrgbl (a->start - 1 + g->spin[s].start + (n + g->spin[s].offset) % g->spin[s].len, 0, 0, max,
                        (n % 3 ? 255 - a->step : 255) * q / 255);
         if ((a->step += 255 / a->speed) > 255)
         {
            a->step = 0;
            a->stage = 10;
         }
         break;
   } else if (a->stage < 100)
      switch (a->stage % 10)
      {                         // 10 to 90 for chevrons 1 to 9
      case 0:                  // Spin spins to position
         {
            uint8_t target = g->dial[a->stage / 10 - 1];
            int8_t dir = -1;
            if ((g->pos < target && g->pos + 18 >= target) || (g->pos > target && target + 18 < g->pos))
               dir = 1;
            if (dir == -1)
               spinner (a->step);       // Yes direction is opposite as we are moving symbol to top
            else
               spinner (3 - a->step);
            a->step++;
            if (a->step == 3)
            {
               g->pos += dir;
               if (!g->pos)
                  g->pos = 39;
               else if (g->pos == 40)
                  g->pos = 1;
               a->step = 0;
               if (target == g->pos)
                  a->stage++;
            }
            chevs ();
            break;
         }
      case 1:                  // Engage top chevron and gate symbol
         spinner (0);
         chev (8, (g->chevs - 1) * (255 - a->step) / 255, g->chevs - 1, q);
         gates (a->stage / 10 - 1, a->step * q / 255);
         if ((a->step += 255 / a->speed) > 255)
         {
            a->step = 0;
            a->stage++;
            if (!g->dial[a->stage / 10])
            {
               a->stage = 100;  // Last one
               a->step = a->fade;
               memset (new, 255, spinlen);
               twinkle ();
            }
         }
         chevs ();
         break;
      case 2:                  // Disengage top chevron
         spinner (0);
         chev (8, (g->chevs - 1) * a->step / 255, g->chevs - 1, q);
         gates (a->stage / 10 - 1, q);
         if ((a->step += 255 / a->speed) > 255)
         {
            a->step = 0;
            a->stage++;
         }
         chevs ();
         break;
      case 3:                  // Light up selected chevron
         spinner (0);
         chev (a->stage / 10 - 1, 0, g->chevs - 1, a->step * q / 255);
         gates (a->stage / 10 - 1, q);
         if ((a->step += 255 / a->speed) > 255)
         {
            a->step = 0;
            a->stage += 7;
         }
         chevs ();
         break;
   } else
   {
      for (int i = 0; i < spinlen; i++)
      {
         uint8_t l = (int) (a->fade - a->step) * new[i] / a->fade + (int) a->step * old[i] / a->fade;
         setrgbl (a->start - 1 + g->spin[0].start + i, l, l, l, q);
      }
      if (!--a->step)
      {                         // Next
         a->step = a->fade;
         twinkle ();
      }
   }
   return NULL;
}

const char *
appstargate (app_t * a)
{
   if (a->len == 210 || a->len == 372)
      return biggate (a);
   if (!a->cycle)
   {
      free (a->data);           // Not used supplied
      a->data = malloc (a->len * 2 + 1);
   }
   uint8_t *old = a->data,
      *new = old + a->len,
      *posp = new + a->len,
      pos = *posp;
   uint8_t top;
   int8_t dir = 1;
   if (a->top < 0)
   {
      top = -a->top - a->start;
      dir = -1;
   } else
      top = a->top - a->start;

   if (!a->cycle)
   {                            // Sanity checks, etc
      if (!a->limit)
         a->limit = 60 * cps;
      a->step = a->fade;
      pos = esp_random ();
      if (!a->colourset)
         a->b = 63;             // Default ring blue
   }
   uint8_t q = 255;
   if (a->stop)
      q = 255 * a->stop / a->fade;

   void ring (uint8_t l)
   {
      for (unsigned int i = 0; i < a->len; i++)
         setl (a->start + i, a, (int) l * wheel[(256 * i / a->len + pos) & 255] / 255);
   }
   void chevron (uint8_t n, uint8_t l)
   {
      if (l > q)
         l = q;
      switch (n)
      {
      case 0:
         n = top;
         break;
      case 1:
         n = (top + a->len - dir * 4 * a->len / 39) % a->len;
         break;
      case 2:
         n = (top + a->len + dir * 4 * a->len / 39) % a->len;
         break;
      case 3:
         n = (top + a->len - dir * 8 * a->len / 39) % a->len;
         break;
      case 4:
         n = (top + a->len + dir * 8 * a->len / 39) % a->len;
         break;
      case 5:
         n = (top + a->len - dir * 12 * a->len / 39) % a->len;
         break;
      case 6:
         n = (top + a->len + dir * 12 * a->len / 39) % a->len;
         break;
      case 7:
         n = top;
         break;
      default:
         return;
      }
      setrgbl (a->start + n, 255, 255, 0, l);
   }
   void twinkle (void)
   {
      memcpy (old, new, a->len);
      esp_fill_random (new, a->len);
      for (int i = 0; i < a->len; i++)
         new[i] = new[i] / 4 + 32;
   }

   if (!a->stage)
   {                            // Fade up
      ring (255 * (a->fade - a->step) / a->fade);
      if (!--a->step)
      {
         a->stage = 10;
         a->step = a->speed + esp_random () % (a->speed * 2);
      }
   } else if (a->stage < 100)
   {                            // Dialling
      ring (255);
      for (int i = 1; i < a->stage / 10; i++)
         chevron (i, q);
      if (!(a->stage % 10))
      {                         // Dial
         if ((a->stage / 20) % 2)
            pos += 256 / a->speed / SPEED;
         else
            pos -= 256 / a->speed / SPEED;
         if (!--a->step)
         {
            a->stage++;
            a->step = a->fade;
         }
      } else if ((a->stage % 10) == 1)
      {                         // Lock top
         chevron (0, 255 * (a->fade - a->step) / a->fade);
         if (!--a->step)
         {
            a->stage++;
            a->step = a->fade;
         }
      } else if (a->stage == 72)
      {                         // Open gate
         chevron (0, q);
         uint8_t l = 255 * (a->fade - a->step) / a->fade;
         for (int i = 0; i < a->len; i++)
         {
            if (getr (a->start + i) < l)
               setr (a->start + i, l);
            if (getg (a->start + i) < l)
               setg (a->start + i, l);
            if (getb (a->start + i) < l)
               setb (a->start + i, l);
         }
         if (!--a->step)
         {                      // Next chevron
            a->stage = 100;
            a->step = a->fade;
            memset (new, 255, a->len);
            twinkle ();
         }
      } else if ((a->stage % 10) == 2)
      {                         // Chevron
         chevron (0, 255 * a->step / a->fade);
         chevron (a->stage / 10, 255 * (a->fade - a->step) / a->fade);
         if (!--a->step)
         {                      // Next chevron
            a->stage += 8;
            a->step = a->speed + esp_random () % (a->speed * 2);
         }
      }
   } else
   {                            // Twinkling
      for (int i = 0; i < a->len; i++)
      {
         uint8_t l = (int) (a->fade - a->step) * new[i] / a->fade + (int) a->step * old[i] / a->fade;
         setrgbl (a->start + i, l, l, l, q);
      }
      if (!--a->step)
      {                         // Next
         a->step = a->fade;
         twinkle ();
      }
   }
   *posp = pos;
   return NULL;
}
