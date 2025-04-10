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

RUN g++ hashsearch.cpp -lfcgi++ -lfcgi -O2 -o hashsearch

COPY entrypoint.sh /app/entrypoint.sh

EXPOSE 8000

CMD ["./entrypoint.sh"]