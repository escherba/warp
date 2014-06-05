#!/bin/bash

infile=$1
extension="${infile##*.}"
filename="${infile%.*}"

hcolor="black"
vcolor="black"
thickness=2
xspace=24
yspace=24

ww=`convert $infile -format "%w" info:`
hh=`convert $infile -format "%h" info:`
inname=`convert $infile -format "%t" info:`
convert $infile \
    \( -size ${xspace}x${yspace} xc:none -background none -fill none -strokewidth $thickness \
    -draw "stroke $vcolor line 0,0 0,$((yspace-1)) stroke $hcolor line 0,0 $((xspace-1)),0" -write mpr:grid +delete \
    -size $((ww))x$((hh)) tile:mpr:grid \) -compose over -composite ${filename}_grid.${extension}
