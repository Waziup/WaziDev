FROM ubuntu:18.04
ENV ARDUINO_CLI_VERSION 0.3.6-alpha.preview
ENV ARDUINO_CLI_ARCH linux64
ENV ARDUINO_CLI_SHA256 d3cf2391e0603d15494754a14e5c8e2a4ae40135e85b78a6b178c3528bd563e3
ENV ARDUINO_CLI_URL https://github.com/arduino/arduino-cli/releases/download/${ARDUINO_CLI_VERSION}/arduino-cli-${ARDUINO_CLI_VERSION}-${ARDUINO_CLI_ARCH}.tar.bz2

# Temporary fix – see issue here: https://github.com/arduino/arduino-cli/issues/182
# by SConaway (https://github.com/SConaway)
ENV USER root

RUN set -eux; \
        \
        apt-get update; \
        apt-get install -y wget; \
        wget -O /arduino-cli.tar.bz2 "$ARDUINO_CLI_URL"; \
        echo "$ARDUINO_CLI_SHA256 */arduino-cli.tar.bz2" | sha256sum -c -; \
        mkdir "/arduino-cli"; \
        tar --extract --file /arduino-cli.tar.bz2 --directory /arduino-cli; \
        rm /arduino-cli.tar.bz2; \
        cd /usr/bin; \
        ln -s /arduino-cli/arduino-cli-${ARDUINO_CLI_VERSION}-${ARDUINO_CLI_ARCH} arduino-cli; \  
# basic test
        cd /; \
        arduino-cli version; \
# install enviroment
        arduino-cli core update-index; \
        arduino-cli core install arduino:avr 

