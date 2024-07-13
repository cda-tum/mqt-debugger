"""The main adapter module that can be run from outside the package to start the server."""

from __future__ import annotations

from .dap_server import DAPServer

if __name__ == "__main__":
    server = DAPServer()
    server.start()
