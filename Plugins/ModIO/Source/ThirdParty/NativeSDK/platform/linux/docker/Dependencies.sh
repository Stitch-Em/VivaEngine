#!/bin/bash

##########################
## Step 1: Compile the Docker image that can run IMGui
docker build -t thegooi -f  DockerfileUI .
##########################

##########################
## Step 2: Execute the Docker image
## NOTE: The command below expects to have a folder named "$(pwd)/modio-sdk-internal" from where it is executed
docker run -it --rm --name ubvnc -p 6080:80 -p 5900:5900 -p 6082:22 -e RESOLUTION=1600x900 -v /dev/shm:/dev/shm -v $(pwd)/modio-sdk-internal:/modio-sdk-internal --security-opt seccomp=unconfined thegooi &
##########################

##########################
## Step 3: Connect to the image. There are two options: via SSH (no UI) or VNC (with UI), outside of this script.
## Via SSH and password = "pass"
expect -c 'spawn ssh -p 6082 root@localhost; expect \"password:\"; send \"pass\r\"; interact'
## Note, if the "expect" command is not present, you can ssh normaly
# ssh -p 6082 root@localhost
##########################

##########################
## Step 4: Compile the gui_test
cd /modio-sdk-internal
cmake -S . -B out/ -G Ninja \
    -DMODIO_PLATFORM=LINUX \
    -DMODIO_BUILD_DOCS=False \
    -DMODIO_BUILD_TESTS=True \
    -DCMAKE_BUILD_TYPE=DEBUG
cmake --build out --target "gui_test" -j 1
##########################

## NOTE!! If the compilation of "gui_test" fails because one of the following is not defined:
## MODIO_UNITTESTS_API_KEY
## MODIO_UNITTESTS_GAME_ID
## MODIO_UNITTESTS_SERVER
## It would be necessary to add them at cmake configuration stage or in the CMakeList:
## cmake -S . -B out/ -G Ninja \
##     -DMODIO_PLATFORM=LINUX \
##     -DMODIO_BUILD_DOCS=False \
##     -DMODIO_BUILD_TESTS=True \
##     -DCMAKE_BUILD_TYPE=DEBUG \
##     -DMODIO_UNITTESTS_API_KEY="API-KEY" \
##     -DMODIO_UNITTESTS_GAME_ID="GAME-ID" \
##     -DMODIO_UNITTESTS_SERVER="Modio::Environment::Test"
