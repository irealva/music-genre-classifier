#! /bin/sh
for file in mftext/midi/*
do
    mftext/mftext "$file" | meter/meter | harmony/harmony | key/key | tr -d '\n' > "$file".txt
done
