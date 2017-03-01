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

The service inits itself so theres nothing to do in your main.c, just make sure you utilize an adc driver, such as adc_nrf51_driver, and follow any instructions included there so that the appropriate adc (adc0 by default) is ready at sysinit time.

You might want to override the time between samples and adc name to use in your target or app syscfg.yml
```
syscfg.vals:
	BATTERY_SAMPLE_DELAY: 1800
    BATTERY_ADC_NAME: '"adc0"'
```
