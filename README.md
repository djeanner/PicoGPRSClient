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

    passthroughEnabled: When true, allows manual command interaction with AIR780 over USB

## Hardware Requirements

Raspberry Pi Pico (or similar with UART)

AIR780 GSM/GPRS module

USB-to-Serial interface (for logging and manual input)

Optional: onboard LED (e.g., GPIO25 on Pico)

## create and install server

in air780-backend/server.js

```zsh
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



```zsh
# brew install cloudflared
docker run --name air780-server -p 3000:3000 air780-server
cloudflared tunnel --url http://localhost:3000
echo "add three entries to server" 

curl -X POST http://10.113.248.55:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=123"
curl -X POST http://damserv.duckdns.org:3000/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=123"
curl -X POST https://picogprsclient.onrender.com/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=124"
curl -X POST https://picogprsclient.onrender.com/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR780&value=textInput"
curl -X POST https://picogprsclient.onrender.com/submit -H "Content-Type: application/x-www-form-urlencoded" -d "device=AIR78ddddd0&value=textInput"

curl --data "device=AIR780&value=123" https://10.113.248.55:3000/submit
curl --data "device=AIR780&value=123" https://damserv.duckdns.org:3000/submit
curl --data "device=AIR780&value=123" https://picogprsclient.onrender.com/submit
curl --data "device=AIR780&value=125" https://picogprsclient.onrender.com/submit
curl --data "device=AIR780&value=textInput" https://picogprsclient.onrender.com/submit
curl --data "device=AIR78ddddd0&value=textInput" https://picogprsclient.onrender.com/submit


echo "open in browser https://picogprsclient.onrender.com/data or store all data :"
wget https://picogprsclient.onrender.com/data -O test/dataStored_OnRender.json

echo "get single element"
wget "https://picogprsclient.onrender.com/data?index=0" -O test/dataStored0_OnRender.json

echo "get single device remove from main arrays"
wget "https://picogprsclient.onrender.com/data?device=AIR780" -O test/dataStored_AIR780_OnRender.json
wget "http://10.113.248.55:3000/data?device=AIR780" -O test/dataStored_AIR780_OnRender.json


echo "set up duckdns at https://www.duckdns.org/domains"
# 10.113.248.55 
# https://www.duckdns.org/login-github?code=9ae9688cce49768c0f77&state=uhoibougyohouhpyh87yy
# DONT FO THIS . IT WOULD USE TE COMPUTER IP ; NOT THE ROUTER 
# echo url="http://www.duckdns.org/update?domains=damserv&token=e99dd4b7-246f-45cf-8da2-709d5d6b5c00&ip=" | curl -k -o ~/duck.log -K -
echo "look at ip at https://salt.box/2.0/gui/#/internetConnectivity/status"

wget "http://damserv.duckdns.org:3000/data" -O -
wget "http://10.113.248.55:3000/data" -O -
damserv