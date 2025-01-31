"""This module handles running assertion programs on real hardware and using statistical tests to check the results."""

from __future__ import annotations

import argparse
import locale
from pathlib import Path

from . import result_checker, run_preparation


def main() -> None:
    """The main function."""
    parser = argparse.ArgumentParser(description="Compile assertion programs for real hardware.")
    subparsers = parser.add_subparsers(dest="mode", required=True, help="The mode to run the program in.")
    parser.add_argument(
        "--calibration", type=Path, help="The path to a calibration file containing device information."
    )

    # Add the subparser for the preparation mode.
    sub_preparation = subparsers.add_parser(
        "prepare", help="Prepare the assertion program for running on real hardware."
    )
    sub_preparation.add_argument(
        "code", type=Path, help="The path to the assertion program in extended OpenQASM format."
    )
    sub_preparation.add_argument(
        "--output-dir", "-o", type=Path, help="The directory to store the compiled slices.", default="."
    )

    # Add the subparser for the checking mode.
    sub_checker = subparsers.add_parser("check", help="Check the results of the assertion program.")
    sub_checker.add_argument("results", type=Path, help="The path to a JSON file containing all results.")
    sub_checker.add_argument("--dir", "-d", type=Path, help="The path to the compiled program.", default=".")
    sub_checker.add_argument("--slice", "-s", type=int, help="The slice index to check.", default=1)

    args = parser.parse_args()

    if args.mode == "prepare":
        run_preparation.start_compilation(args.code, args.output_dir)
    elif args.mode == "check":
        with (args.dir / f"slice_{args.slice}.qasm").open("r", encoding=locale.getpreferredencoding(False)) as f:
            compiled_code = f.read()
        result_checker.check_result(compiled_code, args.results)
