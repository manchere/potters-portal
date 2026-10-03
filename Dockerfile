# PottersPortalServer (the REST API the mobile/web app talks to) as a Linux
# container, for hosting it online (e.g. Render). Debian 13 ships Qt 6.8,
# the first version with the QHttpServer::bind() the server uses. Qt
# HttpServer's CMake package needs Qt WebSockets, so its -dev is installed
# too or find_package() fails.
#
# Settings come from environment variables at run time:
#   DATABASE_URL  Postgres connection string (required)
#   GROQ_API_KEY  for photo auto-fill (optional; those routes fail without it)
#   PORT          port to listen on (hosts set this; defaults to 8080)

# --- Build -------------------------------------------------------------------
FROM debian:trixie-slim AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build qt6-base-dev qt6-httpserver-dev \
      qt6-websockets-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt ./
COPY src ./src
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPOTTERS_SERVER_ONLY=ON \
 && cmake --build build --target PottersPortalServer

# --- Run ---------------------------------------------------------------------
FROM debian:trixie-slim
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      libqt6httpserver6 libqt6sql6-psql ca-certificates \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --system --no-create-home portal
COPY --from=build /src/build/src/PottersPortalServer /usr/local/bin/PottersPortalServer
USER portal
ENV PORT=8080
EXPOSE 8080
CMD ["PottersPortalServer"]
