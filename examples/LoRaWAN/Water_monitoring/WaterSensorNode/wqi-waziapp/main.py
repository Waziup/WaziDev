#!/usr/bin/env python3
"""
WaziApp: the water node's CCME-WQI as a number and a traffic light.

On the gateway the edge proxies http://<gateway>/apps/kijanispace.wqi/...
to the unix socket proxy.sock in the app's folder, which docker-compose
mounts at /var/lib/waziapp. The app itself reads the node's values from the
edge API on 127.0.0.1: the container runs with the host network, and the
edge lets loopback requests through without a token.

    GET /ui/        the page (ui/index.html)
    GET /api/wqi    the index over the last WINDOW_HOURS, as JSON

Which device: DEVICE_ID if set, otherwise the most recently changed device
that has sensors on the channels named in objectives.json (the water node,
as set up by create_/update_wazigate_device.js).

For development off the gateway:
    python main.py --tcp 8080 --api http://<gateway-ip>
(the edge then wants a token: set WAZIGATE_TOKEN, or point --api at a mock)
"""

import argparse
import json
import os
import socket
import socketserver
import sys
import urllib.parse
import urllib.request
from datetime import datetime, timedelta, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer

import wqi

HERE = os.path.dirname(os.path.abspath(__file__))
APP_DIR = "/var/lib/waziapp"           # the app's folder on the gateway, mounted
API = os.environ.get("WAZIGATE_API", "http://127.0.0.1").rstrip("/")
TOKEN = os.environ.get("WAZIGATE_TOKEN", "")

CONTENT_TYPES = {".html": "text/html; charset=utf-8", ".svg": "image/svg+xml",
                 ".css": "text/css", ".js": "application/javascript", ".png": "image/png"}


def load_config():
    # A copy in the app's folder on the gateway wins, so the limits can be
    # edited there without rebuilding the image.
    for path in (os.path.join(APP_DIR, "objectives.json"), os.path.join(HERE, "objectives.json")):
        if os.path.exists(path):
            return wqi.load_objectives(path)
    raise FileNotFoundError("objectives.json")


def api_get(path):
    req = urllib.request.Request(API + path, headers={"Accept": "application/json"})
    if TOKEN:
        req.add_header("Authorization", "Bearer " + TOKEN)
    with urllib.request.urlopen(req, timeout=15) as resp:
        return json.load(resp)


def sensor_chan(sensor):
    try:
        return int((sensor.get("meta") or {}).get("xlppChan"))
    except (TypeError, ValueError):
        return None


def pick_device(devices, chans):
    wanted = os.environ.get("DEVICE_ID", "").strip()
    if wanted:
        return next((d for d in devices if d.get("id") == wanted), None)
    # The device that carries most of the configured channels; among equals,
    # the most recently changed, since older duplicates no longer get data.
    def score(d):
        have = {sensor_chan(s) for s in d.get("sensors") or []}
        return (len(have & chans), d.get("modified") or "")
    best = max(devices, key=score, default=None)
    return best if best is not None and score(best)[0] > 0 else None


def compute():
    cfg = load_config()
    variables = cfg["variables"]
    hours = float(os.environ.get("WINDOW_HOURS") or cfg.get("window_hours", 24))
    light = cfg.get("traffic_light", {"green": 80, "amber": 65})

    device = pick_device(api_get("/devices"), {v["xlppChan"] for v in variables.values()})
    if device is None:
        return {"error": "no device with the configured sensor channels"}

    since = (datetime.now(timezone.utc) - timedelta(hours=hours)).strftime("%Y-%m-%dT%H:%M:%SZ")
    series = {}
    for name, lim in variables.items():
        sensor = next((s for s in device.get("sensors") or [] if sensor_chan(s) == lim["xlppChan"]), None)
        if sensor is None:
            continue
        path = "/devices/%s/sensors/%s/values?from=%s" % (
            urllib.parse.quote(device["id"]), urllib.parse.quote(sensor["id"]), since)
        series[name] = [v.get("value") for v in api_get(path)]

    result = wqi.ccme_wqi(series, variables) or {"wqi": None, "category": "No data"}
    w = result["wqi"]
    result["light"] = (None if w is None else
                       "green" if w >= light["green"] else
                       "amber" if w >= light["amber"] else "red")
    result.update(device=device.get("name") or device["id"], window_hours=hours,
                  computed=datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"))
    return result


class Handler(BaseHTTPRequestHandler):

    def address_string(self):
        return "edge"                   # a unix socket has no client address

    def log_message(self, fmt, *args):
        sys.stderr.write("[wqi] %s\n" % (fmt % args))

    def reply(self, code, body, ctype):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urllib.parse.urlparse(self.path).path
        if path == "/api/wqi":
            try:
                body, code = compute(), 200
            except Exception as e:      # the page shows it instead of a dead light
                body, code = {"error": "%s: %s" % (type(e).__name__, e)}, 502
            self.reply(code, json.dumps(body).encode(), "application/json")
        elif path.startswith("/ui/") or path == "/ui":
            name = path[4:] or "index.html"
            file = os.path.normpath(os.path.join(HERE, "ui", name))
            if not file.startswith(os.path.join(HERE, "ui")) or not os.path.isfile(file):
                self.reply(404, b"not found", "text/plain")
                return
            with open(file, "rb") as f:
                ext = os.path.splitext(file)[1]
                self.reply(200, f.read(), CONTENT_TYPES.get(ext, "application/octet-stream"))
        elif path == "/":
            self.reply(200, b"WQI WaziApp running. UI at ui/", "text/plain")
        else:
            self.reply(404, b"not found", "text/plain")


class UnixServer(socketserver.ThreadingMixIn, HTTPServer):
    address_family = getattr(socket, "AF_UNIX", None)   # absent on Windows: --tcp only
    daemon_threads = True

    def server_bind(self):
        socketserver.TCPServer.server_bind(self)
        self.server_name, self.server_port = "waziapp", 0


def main():
    global API
    ap = argparse.ArgumentParser(description="WQI WaziApp")
    ap.add_argument("--tcp", type=int, help="serve on localhost:PORT instead of the unix socket")
    ap.add_argument("--api", help="edge API base URL (default %s)" % API)
    args = ap.parse_args()
    if args.api:
        API = args.api.rstrip("/")

    if args.tcp:
        server = socketserver.ThreadingTCPServer(("127.0.0.1", args.tcp), Handler)
        print("[wqi] http://127.0.0.1:%d/ui/  (API %s)" % (args.tcp, API))
    else:
        sock = os.path.join(APP_DIR, "proxy.sock")
        if os.path.exists(sock):
            os.unlink(sock)
        server = UnixServer(sock, Handler)
        os.chmod(sock, 0o666)
        print("[wqi] listening on %s  (API %s)" % (sock, API))
    server.serve_forever()


if __name__ == "__main__":
    main()
