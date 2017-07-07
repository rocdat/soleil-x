#!/bin/bash

if [[ "$1" == "" ]]
then
  SOURCE_DIR=../soleil-x/src/out
else
  SOURCE_DIR=$1
fi

ls -1 ${SOURCE_DIR}/image.*.ppm | sed -e "s:${SOURCE_DIR}/image.::" | sed -e "s:\..*.ppm::" | uniq > .tmp_timesteps
while read TIMESTEP
do
  echo
  echo === composite timestep ${TIMESTEP} ===

# taylor_with_smaller_particles
  CMD="./offline_composite \
    ${SOURCE_DIR}/image.${TIMESTEP}.final.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.0_0_0__127_127_255.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.0_0_0__127_127_255.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.0_128_0__127_255_255.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.0_128_0__127_255_255.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.128_0_0__255_127_255.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.128_0_0__255_127_255.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.128_128_0__255_255_255.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.128_128_0__255_255_255.ppm"

# cavity32x32 
#   ${SOURCE_DIR}/image.${TIMESTEP}.0_0_0__16_16_2.ppm \
#   ${SOURCE_DIR}/depth.${TIMESTEP}.0_0_0__16_16_2.ppm \
#   ${SOURCE_DIR}/image.${TIMESTEP}.0_17_0__16_33_2.ppm \
#   ${SOURCE_DIR}/depth.${TIMESTEP}.0_17_0__16_33_2.ppm \
#   ${SOURCE_DIR}/image.${TIMESTEP}.17_0_0__33_16_2.ppm \
#   ${SOURCE_DIR}/depth.${TIMESTEP}.17_0_0__33_16_2.ppm \
#   ${SOURCE_DIR}/image.${TIMESTEP}.17_17_0__33_33_2.ppm \
#   ${SOURCE_DIR}/depth.${TIMESTEP}.17_17_0__33_33_2.ppm"
  ${CMD} 
  pnmtopng < ${SOURCE_DIR}/image.${TIMESTEP}.final.ppm > ${SOURCE_DIR}/image.${TIMESTEP}.final.png
done < .tmp_timesteps

echo === create video ===
ffmpeg -r 10 -s 3840x2160 -f image2 -i ${SOURCE_DIR}/image.%05d.final.png -crf 25 -vcodec libx264 ${SOURCE_DIR}/video.final.mp4

echo to add a soundtrack do this
echo ffmpeg -i ${SOURCE_DIR}/video.final.mp4 -i <soundtrack> -shortest ${SOURCE_DIR}/video_with_soundtrack.final.mp4
