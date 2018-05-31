
Install git to be able to download source files:

```
  sudo apt-get install git
```

Install the following packages to build source:

```
  sudo apt-get install libfftw3-dev
  sudo apt-get install libpulse-dev
  sudo apt-get install libgtk-3-dev
```

Install the following packages to be able to install the Debian package

```
  sudo apt-get install libfftw3-3
  sudo apt-get install libpulse0
```


To download, compile and install WDSP from https://github.com/g0orx/wdsp

```
  git clone https://github.com/g0orx/wdsp
  cd wdsp
  make
  sudo make install
```

To download, compile and install linHPSDR from https://github.com/g0orx/linhpsdr

```
  git clone https://github.com/g0orx/linhpsdr
  cd linhpsdr
  make
  make install
```


