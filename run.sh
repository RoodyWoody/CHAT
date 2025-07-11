#!/bin/bash

docker-compose up -d

mkdir ./output

docker cp app_container:/usr/src/app/my_app ./output/my_app