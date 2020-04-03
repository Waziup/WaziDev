WaziDev
=======

This repository contains everything you need to use [WaziDev](http://www.waziup.io/documentation/wazidev/).

It can be loaded as a sketchbook in Arduino IDE.

Develop
=======


You can clone this repo with:
```
git clone git@github.com:Waziup/WaziDev.git
```

ATTENTION, this repo use a "subtree" for WaziDev library. The subtree is in libraries/WaziDev.
You can get the last change to the library with:
```
git subtree pull --prefix libraries/WaziDev/ --squash wazidev-lib master
```

If you made changes to the WaziDev library, push them to the original repo of the library:
```
git subtree push --prefix libraries/WaziDev/ --squash wazidev-lib master
```

