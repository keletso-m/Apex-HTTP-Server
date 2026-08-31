# build stage
FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y \
    g++ make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Build only the server binary e
RUN make all

#Runtime stage
FROM ubuntu:24.04 AS runtime

# No compilers
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --no-create-home --shell /usr/sbin/nologin apex

WORKDIR /app

COPY --from=build /src/bin/apex-server ./bin/apex-server
COPY --from=build /src/config ./config
COPY --from=build /src/www ./www

RUN mkdir -p /app/logs && chown -R apex:apex /app

USER apex

EXPOSE 8080

ENTRYPOINT ["./bin/apex-server", "config/server.conf"]