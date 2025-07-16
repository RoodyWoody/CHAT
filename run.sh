#!/bin/bash

docker-compose up -d

docker cp client_container:/usr/src/client/client ./client
docker cp server_container:/usr/src/server/server ./server