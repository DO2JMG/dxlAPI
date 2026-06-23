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

## sondemod

`sondemod` should enable the JSON UDP output, for example:

```
 sondemod ...  -M <ip>:<port>
```
