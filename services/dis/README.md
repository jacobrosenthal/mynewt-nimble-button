# ble_svc_dis

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
    - "@mynewt-nimble-services/services/dis"
```

The service inits itself so theres nothing to do in your main.c
