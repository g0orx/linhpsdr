# LinHPSDR MacOS Support

### Development environment

Development and testing has been run on MacOS Sierra 10.12.6. Prerequisites are installed using [Homebrew](https://brew.sh/).

### Prerequisites for building

```
  brew install fftw
  brew install pulseaudio --with-glib
  brew install gtk+3
  brew install gnome-icon-theme
```

### linhpsdr requires WDSP to be built and installed

```
  git clone https://github.com/g0orx/wdsp.git
  cd wdsp
  make -f Makefile.mac install
```

### To download, compile and install linHPSDR from https://github.com/g0orx/linhpsdr

```
  git clone https://github.com/g0orx/linhpsdr.git
  cd linhpsdr
  make -f Makefile.mac install
```

The build installs linHPSDR into `/usr/local/bin`. To run it, type `linhpsdr` on the command line. 

