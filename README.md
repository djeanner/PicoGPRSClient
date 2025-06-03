# PicoGPRSClient
This Arduino sketch is designed to test and debug GPRS-based HTTP POST requests using the AIR780 module (via AT commands) on platforms like the Raspberry Pi Pico (or compatible boards).

main ino file: src/main.cpp
## Features

    ✅ Verbose debug mode (verbose = true) for rich serial logging

    🔁 Optional LED blink diagnostics to indicate status (success/failure)

    🌐 Sends POST request to httpbin.org with sample payload

    🔧 Supports manual passthrough mode via USB serial

    📡 GPRS configuration with APN setup and IP stack initialization

    🔍 Includes basic diagnostic AT commands for signal, registration, operator, etc.

## Configuration

    verbose: Set to false to reduce serial output

    testHttpBin: Set to true to test response parsing and device echo from httpbin

## Hardware Requirements

Raspberry Pi Pico (or similar with UART)

AIR780 GSM/GPRS module

USB-to-Serial interface (for logging and manual input)

Optional: onboard LED (e.g., GPIO25 on Pico)

## create and install server

For local tests
```zsh

echo "install node modules if needed"
npm install
echo "build server"
docker build --platform=linux/amd64 -t air780-server .

echo "**********************************************************************"
echo "run server locally (not getting data from air780 - only to test server code)"
docker rm -f air780-server 2>/dev/null
docker run --name air780-server -p 3000:3000 air780-server

echo "add three entries to server"
curl -X POST http://localhost:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=123"
curl -X POST http://localhost:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=124"
curl -X POST http://localhost:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=textInput"
curl -X POST http://localhost:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR78ddddd0&value=textInput"

# echo "use '-' instead of filename to dump output to stdout"
# wget http://localhost:3000/data -O -
echo "open in browser http://localhost:3000/data or store all data :"
wget http://localhost:3000/data -O test/dataStored.json

echo "get single element"
wget "http://localhost:3000/data?index=0" -O test/dataStored0.json

echo "get single device remove from main arrays"
wget "http://localhost:3000/data?device=AIR780" -O test/dataStored_AIR780.json

echo "stop server"
docker stop air780-server
docker rm air780-server
echo "List active servers (run docker ps -a)"
docker ps -a  

echo "related commands"
echo "docker container prune ---- if needed after        "
echo "open http://localhost:3000/data in browser → should return [] at start"

```

Install on server accepting http (not https)
we used https://app.jpc.infomaniak.com/

```zsh
# brew install cloudflared
docker run --name air780-server -p 3000:3000 air780-server

echo "add entries to server" 

curl -X POST http://193.134.93.138:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=123"
curl -X POST http://193.134.93.138:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=124"
curl -X POST http://193.134.93.138:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=125"
curl -X POST http://193.134.93.138:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIZZR780&value=0"

echo "open in browser http://193.134.93.138:3000/ or store all data :"
wget http://193.134.93.138:3000/data -O - | jq > test/dataStored.json

echo "get single element"
wget "http://193.134.93.138:3000/data?index=0" -O - | jq >test/dataStored0.json

echo "get single device remove from main arrays add incremenetally to test/dataStored_AIR780.json "
wget "http://193.134.93.138:3000/data?device=AIR780" -O  test/dataStored_AIR780_tmp.json;
jq -s '.[0] + .[1]' test/dataStored_AIR780.json test/dataStored_AIR780_tmp.json > test/tmp && mv test/tmp test/dataStored_AIR780.json
rm test/dataStored_AIR780_tmp.json test/tmp

```

On jpc (formerly jelastic)
have a Docker Engine CE, with a Engine Node inside
From the later click the terminal icon "web ssh"
and paste all the following:
```

git clone https://github.com/djeanner/PicoGPRSClient.git
cd PicoGPRSClient
docker build --platform=linux/amd64 -t air780-server .
docker run -d --name air780-server -p 3000:3000 air780-server
echo "check running:"
docker ps
```

to update  ...

```

cd PicoGPRSClient
git pull
docker stop air780-server
docker rm air780-server
docker build -t air780-server .
docker run -d --name air780-server -p 3000:3000 air780-server
```
