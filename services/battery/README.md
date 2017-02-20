# ble_svc_battery

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
    - "@mynewt-nimble-services/services/battery"
```

In your main.c include the header, init an adc osdriver, probably from a package like adc_nrf51_driver, and init the battery
```
#include "battery/ble_svc_battery.h"


#include <adc/adc.h>
#include "adc_nrf51_driver/adc_nrf51_driver.h"

int
main(void)
{
...
    adc = adc_nrf51_driver_init();
    rc = ble_svc_battery_init(adc);
...
}
```

You might want to override the time between samples in your target or app syscfg.yml
```
syscfg.vals:
	BATTERY_SAMPLE_DELAY:1800
```
