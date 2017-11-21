#########################################################################
# File Name:maskconv.sh
# Author: sudoku.huang
# mail: sudoku.huang@gmail.com
# Created Time:Thu 12 Oct 2017 04:25:08 PM CST
#########################################################################
#!/bin/bash

function transparent() 
{
    echo "change mask to transparent"
    date
    for file in mask/*
    do
        #echo $file
        dstfile=${file##*/}
        #echo $dstfile
        convert $file -fuzz 50% -transparent black masktransparent/$dstfile
        #break
    done
    date
}

function mask()
{
    echo "decompress mask to png"
    date
    ffmpeg -i asset0.mp4 -f image2 mask/%04d.png
    date
}

function sucai()
{
    echo "decompress sucai to png"
    date
    ffmpeg -i asset1.mp4 -f image2 sucai/%04d.png
    date
}

function merge1()
{
    echo "merge 1"
    date
    for file in mask/*
    do
        #echo $file
        name=${file##*/}
        convert masktransparent/$name -compose atop sucai/$name -composite result/$name
    done
    date
}

function merge2()
{
    echo "merge 2"
    mkdir out
    date
    for file in result/*
    do
        #echo $file
        name=${file##*/}
        convert bg.png  -compose atop result/$name -composite out/$name
    done
    date
}

function finally()
{
    echo "finally"
    date
    ffmpeg -f image2 -i out/%04d.png -vcodec libx264 -r 15 out.mp4
    date
}

#mask
#sucai
#transparent
#merge1
#merge2
finally
