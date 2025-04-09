FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    spawn-fcgi \
    fcgiwrap \
    curl \
    g++ \
    make \
    libfcgi-dev \
    libfcgi++-dev \
    p7zip-full

WORKDIR /app

COPY . /app

RUN curl -L https://github.com/glacier-modding/Hitman-Hashes/releases/latest/download/latest-hashes.7z -o latest-hashes.7z && \
    7z x latest-hashes.7z -y && \
    rm -rf latest-hashes.7z

RUN g++ hashsearch.cpp -lfcgi++ -lfcgi -O2 -o hashsearch

EXPOSE 8000

CMD ["spawn-fcgi", "-p", "8000", "./hashsearch", "-n"]
