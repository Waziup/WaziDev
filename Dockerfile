#Build stage
FROM tombenke/darduino

USER root
RUN apt-get update && apt-get install -y make arduino-mk 

USER developer

