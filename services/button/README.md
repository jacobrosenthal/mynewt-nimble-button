# ble_svc_button

Add the repo to your project.yml
```
project.repositories:
    - apache-mynewt-core
    - mynewt-nimble-services

repository.mynewt-nimble-services:
    type: github
    vers: 0-latest
    user: jacobrosenthal
    repo: mynewt-nimble-services
```

Add the dependency to your pkg.yml
```
    - "@mynewt-nimble-services/services/button"
```

In your app or target syscfg.yml you need to set the pin number, and can optionally define if the button is inverted or needs a pullup
```
syscfg.vals:
    BUTTON_PIN: 17
    BUTTON_PULLUP: 'HAL_GPIO_PULL_NONE'
    BUTTON_INVERTED: 1
```

In your main.c include the header. The service inits itself, but you can optionally set a callback to receive the event:
```
/* Button */
#include "button/ble_svc_button.h"

static void
bleprph_on_pressed(struct os_event *ev)
{
    uint32_t count = ble_svc_button_count();

    BLEPRPH_LOG(INFO, "bleprph_on_pressed pressed %d times", count);
}

int
main(void)
{
...
	ble_svc_button_register_handler(bleprph_on_pressed);
...
}
```
