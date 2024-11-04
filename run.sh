#!/bin/bash

name=$(./build.sh $1)
echo $2 | ./$name