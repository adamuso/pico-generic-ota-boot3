#!/bin/sh

sudo stty -F /dev/ttyACM0 115200 raw
sudo cat build/bare_blink.bin > /dev/ttyACM0
