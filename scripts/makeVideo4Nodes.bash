#!/bin/bash

if [[ "$1" == "" ]]
then
  OUTDIR=out
else
  OUTDIR=$1
fi

echo == Make MP4 videos from PPM images in dir ${OUTDIR} ===
date

TMP_RAW_IMAGES=${OUTDIR}/.ppm_images
rm -f ${TMP_RAW_IMAGES}
ls -1 ${OUTDIR}/image.*.ppm | sed -e "s:${OUTDIR}/::" | sed -e "s:.ppm::" > ${TMP_RAW_IMAGES}

echo == Convert ppm images to png ===
wc -l ${TMP_RAW_IMAGES}
while read PPM_IMAGE
do
  echo Convert ${PPM_IMAGE}
  pnmtopng < ${OUTDIR}/${PPM_IMAGE}.ppm > ${OUTDIR}/${PPM_IMAGE}.png
done < ${TMP_RAW_IMAGES}

date

echo === Combine png images into .mp4 video ===
for SUBDOMAIN in 0_0_0__16_16_2 0_17_0__16_33_2 17_0_0__33_16_2 17_17_0__33_33_2
do
  ffmpeg -r 10 -s 3840x2160 -f image2 -i ${OUTDIR}/image.%05d.${SUBDOMAIN}.png -crf 25 -vcodec libx264 ${OUTDIR}/video.${SUBDOMAIN}.mp4
done

date
