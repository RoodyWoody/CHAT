#!/bin/bash

docker-compose up -d

mkdir ./output

docker cp client_container:/usr/src/client/client ./output/client
docker cp server_container:/usr/src/server/server ./output/server