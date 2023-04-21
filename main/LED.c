/* LED app */
/* Copyright �2019 - 23 Adrian Kennard, Andrews & Arnold Ltd.See LICENCE file for details .GPL 3.0 */

static const char TAG[] = "LED";

#include "revk.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include <driver/gpio.h>
#include <driver/uart.h>
#include "led_strip.h"
#include "app.h"

#define a(app)	extern const char* app(app_t*);
#include "apps.h"

struct applist_s
{
   const char *appname;
   app_f *app;
} applist[] = {
#define a(app)	{#app,&app},
#include "apps.h"
};

uint8_t *ledr = NULL;
uint8_t *ledg = NULL;
uint8_t *ledb = NULL;

#define u32(n,d)        uint32_t n;
#define u32l(n,d)        uint32_t n;
#define s8(n,d) int8_t n;
#define s8n(n,d) int8_t n[d];
#define u8(n,d) uint8_t n;
#define u8l(n,d) uint8_t n;
#define b(n) uint8_t n;
#define s(n) char * n;
#define io(n,d)           uint8_t n;
settings
#undef io
#undef u32
#undef u32l
#undef s8
#undef s8n
#undef u8
#undef u8l
#undef b
#undef s
   uint8_t gatedial = 0;

const uint8_t cos256[256] =
   { 255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 252, 251, 250, 249, 248, 247, 246, 245, 243, 242, 240, 239, 237, 236, 234,
   232, 230, 228, 226, 224, 222, 220, 218, 216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 191, 188, 185, 182, 179, 176, 174, 171,
   168, 165, 162, 159,
   156, 152, 149, 146, 143, 140, 137, 134, 131, 127, 124, 121, 118, 115, 112, 109, 106, 103, 99, 96, 93, 90, 87, 84, 81, 79, 76, 73,
   70, 67, 64, 62, 59,
   56, 54, 51, 49, 46, 44, 42, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 18, 16, 15, 13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1,
   0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 16, 18, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 42, 44, 46, 49,
   51, 54, 56, 59, 62,
   64, 67, 70, 73, 76, 79, 81, 84, 87, 90, 93, 96, 99, 103, 106, 109, 112, 115, 118, 121, 124, 127, 131, 134, 137, 140, 143, 146,
   149, 152, 156, 159, 162,
   165, 168, 171, 174, 176, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216, 218, 220, 222, 224, 226, 228,
   230, 232, 234, 236,
   237, 239, 240, 242, 243, 245, 246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255, 255, 255, 255, 255
};

const uint8_t wheel[256] =
   { 0, 0, 0, 0, 1, 2, 3, 4, 5, 7, 8, 10, 12, 14, 16, 19, 21, 24, 27, 30, 33, 36, 40, 43, 47, 50, 54, 58, 62, 66, 70, 75, 79, 83,
   88, 92, 97, 102, 106, 111, 116, 120, 125, 130, 135, 139, 144, 149, 153, 158, 163, 167, 172, 176, 180, 185, 189, 193, 197, 201,
   205, 208, 212, 215,
   219, 222, 225, 228, 231, 234, 236, 239, 241, 243, 245, 247, 248, 250, 251, 252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 254,
   253, 252, 251, 250,
   248, 247, 245, 243, 241, 239, 236, 234, 231, 228, 225, 222, 219, 215, 212, 208, 205, 201, 197, 193, 189, 185, 180, 176, 172, 167,
   163, 158, 153, 149,
   144, 139, 135, 130, 125, 120, 116, 111, 106, 102, 97, 92, 88, 83, 79, 75, 70, 66, 62, 58, 54, 50, 47, 43, 40, 36, 33, 30, 27, 24,
   21, 19, 16, 14, 12,
   10, 8, 7, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0
};

const char *
app_callback (int client, const char *prefix, const char *target, const char *suffix, jo_t j)
{
   if (client || !prefix || target || strcmp (prefix, prefixcommand) || !suffix)
      return NULL;              // Not for us or not a command from main MQTT
   // TODO command to set active apps

   return NULL;
}

void
led_task (void *x)
{
   ESP_LOGI (TAG, "Started using GPIO %d%s", ledgpio & 63, ledgpio & 64 ? " (inverted)" : "");

   led_strip_handle_t strip = NULL;

   led_strip_config_t strip_config = {
      .strip_gpio_num = (ledgpio & 63),
      .max_leds = leds,         // The number of LEDs in the strip,
      .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
      .led_model = LED_MODEL_WS2812,    // LED strip model
      .flags.invert_out = ((ledgpio & 64) ? 1 : 0),     // whether to invert the output signal (useful when your hardware has a level inverter)
   };

   led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,   // different clock source can lead to different power consumption
      .resolution_hz = 10 * 1000 * 1000,        // 10MHz
      .flags.with_dma = false,  // whether to enable the DMA feature
   };
   REVK_ERR_CHECK (led_strip_new_rmt_device (&strip_config, &rmt_config, &strip));

   REVK_ERR_CHECK (led_strip_clear (strip));

   ledr = calloc (leds, sizeof (*ledr));
   ledg = calloc (leds, sizeof (*ledg));
   ledb = calloc (leds, sizeof (*ledb));

#define	MAXAPPS 10
   app_t active[MAXAPPS] = { 0 };
   active[0].name = "spin";
   active[0].app = spin;        // Dummy start
   active[0].rainbow2 = 1;

   if (!cps)
      cps = 10;
   uint32_t tick = 1000000LL / cps;

   while (1)
   {                            // Main loop
      usleep (tick - (esp_timer_get_time () % tick));
      clear (1, leds);
      for (unsigned int i = 0; i < MAXAPPS; i++)
         if (active[i].app)
         {
            if (active[i].delay)
            {                   // Delayed start
               active[i].delay--;
               continue;
            }
            if (active[i].rainbow)
            {                   // Cycle the colour
               active[i].r = wheel[(active[i].cycle) & 255];
               active[i].g = wheel[(active[i].cycle + 85) & 255];
               active[i].b = wheel[(active[i].cycle + 170) & 255];
            } else if (active[i].rainbow2)
            {                   // Cycle the colour
               active[i].r = cos256[(active[i].cycle) & 255];
               active[i].g = cos256[(active[i].cycle + 85) & 255];
               active[i].b = cos256[(active[i].cycle + 170) & 255];
            }
            const char *e = active[i].app (&active[i]);
            if (e)
            {
               active[i].app = NULL;    // Done
               if (*e)
               {
                  ESP_LOGI (TAG, "App failed %s", e);   // TODO report via MQTT
               }
            }
            active[i].cycle++;
            if (active[i].time && active[i].cycle >= active[i].time)
               active[i].app = NULL;    // Complete
         }

      for (unsigned int i = 0; i < leds; i++)
      {
         led_strip_set_pixel (strip, i, (unsigned int) bright * ledr[i] / 255, (unsigned int) bright * ledg[i] / 255,
                              (unsigned int) bright * ledb[i] / 255);
      }
      REVK_ERR_CHECK (led_strip_refresh (strip));
   }
}

void
app_main ()
{
   revk_boot (&app_callback);
#define io(n,d)           revk_register(#n,0,sizeof(n),&n,"- "#d,SETTING_SET|SETTING_BITFIELD);
#define b(n) revk_register(#n,0,sizeof(n),&n,NULL,SETTING_BOOLEAN);
#define u32(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u32l(n,d) revk_register(#n,0,sizeof(n),&n,#d,SETTING_LIVE);
#define s8(n,d) revk_register(#n,0,sizeof(n),&n,#d,SETTING_SIGNED);
#define s8n(n,d) revk_register(#n,d,sizeof(*n),&n,NULL,SETTING_SIGNED);
#define u8(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u8l(n,d) revk_register(#n,0,sizeof(n),&n,#d,SETTING_LIVE);
#define s(n) revk_register(#n,0,0,&n,NULL,0);
   settings
#undef io
#undef u32
#undef u32l
#undef s8
#undef s8n
#undef u8
#undef u8l
#undef b
#undef s
      revk_start ();

   revk_task ("LED", led_task, NULL);
}
