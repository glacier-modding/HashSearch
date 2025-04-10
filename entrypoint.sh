#!/bin/bash
set -e

echo "Downloading latest hash list..."
curl -L https://github.com/glacier-modding/Hitman-Hashes/releases/latest/download/latest-hashes.7z -o latest-hashes.7z
7z x latest-hashes.7z -y
rm latest-hashes.7z

echo "Starting HashSearch..."
exec spawn-fcgi -p 8000 ./hashsearch -n
