#!/bin/bash
convert -background none -fill white -font font.ttf -pointsize 22 label:"\ \!\"#$\%\&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\]^_\`abcdefghijklmnopqrstuvwxyz{|}~" font.png &&
    convert -background none -fill black -font font.ttf -pointsize 10 label:"\ \!\"#$\%\&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\]^_\`abcdefghijklmnopqrstuvwxyz{|}~" smallfont.png
