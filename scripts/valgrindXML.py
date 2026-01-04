#!/usr/bin/env python3
from pathlib import Path
import xml.etree.ElementTree as ET
import argparse

def print_errors(errors):
    seen = set()  # to track (error_type, file_line) and avoid duplicates
    for error_type, file_line, description in errors:
        if (error_type, file_line) in seen:
            continue  # skip duplicates
        seen.add((error_type, file_line))

        # First line: Error Type and File:Line
        print(f"{error_type:<15} | {file_line}")
        # Second line: Description indented
        if description:
            print(f"{'':17} {description}")
        print()  # blank line between errors

def parse_valgrind_xml(xml_file: Path, exclude_folders=None):
    if not xml_file.exists():
        print(f"Error: XML file not found: {xml_file}", flush=True)
        return

    tree = ET.parse(xml_file)
    root = tree.getroot()

    exclude_folders = exclude_folders or []
    errors = []

    for error in root.findall('error'):
        error_type = error.findtext('kind') or "Unknown"

        file_line = "Unknown"
        frames = error.findall('stack/frame')
        for frame in frames:
            file_name = frame.findtext('file')
            dir_path = frame.findtext('dir')
            line = frame.findtext('line')
            if file_name and dir_path and line:
                abs_path = (Path(dir_path) / file_name).resolve()
                # Skip if path contains excluded folder
                if any(ex in str(abs_path) for ex in exclude_folders):
                    file_line = None
                    break
                file_line = f"{abs_path}:{line}"
                break  # first valid frame

        # Skip error if file_line is None (excluded)
        if file_line is None:
            continue

        description = error.findtext('auxwhat') or error.findtext('what') or ""

        errors.append((error_type, file_line, description))

    print_errors(errors)

def main():
    parser = argparse.ArgumentParser(description="Parse Valgrind XML and print errors.")
    parser.add_argument("xml_file", nargs="?", help="Path to valgrind XML file")
    parser.add_argument("--exclude-folders", nargs="*", default=["/workspace/build/"], 
                        help="List of folder names to exclude errors from (e.g., build)")
    args = parser.parse_args()

    xml_file = Path(args.xml_file) if args.xml_file else (Path(__file__).resolve().parent / "../build/tests/valgrind.xml").resolve()
    if not xml_file.exists():
        print(f"Error: XML file not found: {xml_file}", flush=True)
        return

    parse_valgrind_xml(xml_file, exclude_folders=args.exclude_folders)

if __name__ == "__main__":
    main()
