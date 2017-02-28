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

The service inits itself so theres nothing to do in your main.c

You might want to override the time between samples and adc name to use in your target or app syscfg.yml
```
syscfg.vals:
	BATTERY_SAMPLE_DELAY: 1800
    BATTERY_ADC_NAME: '"adc0"'
```
