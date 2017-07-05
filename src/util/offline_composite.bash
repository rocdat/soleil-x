#!/bin/bash
SOURCE_DIR=../soleil-x/src/out
ls ${SOURCE_DIR}/image.*.ppm | sed -e "s:../soleil-x/src/out/image.::" | sed -e "s:\..*.ppm::" | uniq > .tmp_timesteps
while read TIMESTEP
do
  echo
  echo === composite timestep ${TIMESTEP} ===
  CMD="./offline_composite \
    ${SOURCE_DIR}/image.${TIMESTEP}.final.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.0_0_0__16_16_2.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.0_0_0__16_16_2.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.0_17_0__16_33_2.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.0_17_0__16_33_2.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.17_0_0__33_16_2.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.17_0_0__33_16_2.ppm \
    ${SOURCE_DIR}/image.${TIMESTEP}.17_17_0__33_33_2.ppm \
    ${SOURCE_DIR}/depth.${TIMESTEP}.17_17_0__33_33_2.ppm"
  ${CMD} 
  pnmtopng < ${SOURCE_DIR}/image.${TIMESTEP}.final.ppm > ${SOURCE_DIR}/image.${TIMESTEP}.final.png
done < .tmp_timesteps

echo === create video ===
ffmpeg -r 10 -s 3840x2160 -f image2 -i ${SOURCE_DIR}/image.%05d.final.png -crf 25 -vcodec libx264 ${SOURCE_DIR}/video.final.mp4
