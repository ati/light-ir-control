#include <stdint.h> /* uint*_t types */


typedef struct {
  int8_t dir;     // -1, 1
  uint8_t is_on;  // 0, 1
  uint8_t left;   // 0 - num_brightness_steps
  uint8_t right;  // 0 - num_brightness_steps
} Brightness;


// python: [int(255.0*(1 - math.log((50 - i), 50))) for i in range(50)]

extern uint8_t brightness_steps[24];
extern const uint32_t keys_up[924];
extern const uint32_t keys_down[946];
extern const uint32_t keys_toggle[1490];

