#!/bin/bash

if ! [ -d build ] ; then
  mkdir build
fi

if ! [ -d output ] ; then
  mkdir output
fi


cd build

cmake ..

make && ./pdf_barcode ../sample-forms/label.pdf ../output/output1.pdf ../output/output1.log 1
make && ./pdf_barcode ../sample-forms/label.pdf ../output/output2.pdf ../output/output2.log 0

cd -
