#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/mlgbcoin.png
ICON_DST=../../src/qt/res/icons/mlgbcoin.ico
convert ${ICON_SRC} -resize 16x16 mlgbcoin-16.png
convert ${ICON_SRC} -resize 32x32 mlgbcoin-32.png
convert ${ICON_SRC} -resize 48x48 mlgbcoin-48.png
convert mlgbcoin-16.png mlgbcoin-32.png mlgbcoin-48.png ${ICON_DST}

