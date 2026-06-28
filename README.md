# dxlAPI

`dxlapi` receives telemetry from dxlAPRS / `sondemod` via UDP JSON and forwards it as JSON POST requests to wettersonde.net.

## Key behavior

- Receives `sondemod -J` UDP JSON packets
- Uploads telemetry to `http://api.wettersonde.net/telemetrie.php`
- Mandatory receiver position upload to `http://api.wettersonde.net/position.php` at startup
- Receiver position is uploaded again every 30 minutes

## Dependencies

Debian / Raspberry Pi OS:

```bash
sudo apt update
sudo apt install build-essential libcurl4-openssl-dev
```

## Build

```bash
make clean
make
```

## Command-line options

Options use a single `-` prefix:

```text
-callsign CALLSIGN
-lat LAT
-lon LON
-bind ADDR
-port PORT
```

```text
Example dxlapi -callsign DO1XYZ -lat 52.12345 -lon 8.12345 -bind 127.0.0.1 -port 18005
```

## sondemod

`sondemod` should enable the JSON UDP output, for example:

```
 sondemod ...  -J <ip>:<port>
```

```text
Example -J 127.0.0.1:18005
```
