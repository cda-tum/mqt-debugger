"""Run a single instance of the DAP server."""

from __future__ import annotations

from mqt.debug import DAPServer


def main() -> None:
    """The main entry point of the DAP server test."""
    server = DAPServer()
    server.start()


if __name__ == "__main__":
    main()
