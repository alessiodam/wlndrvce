#!/usr/bin/env python3
import argparse
import json
import os
import sys
from typing import List

try:
    from tivars.types import TIAppVar
    from tivars.var import TIHeader
except ImportError:
    print(
        "Error: 'tivars' library not found. Please install it via 'pip install tivars'.",
        file=sys.stderr,
    )
    sys.exit(1)

CHUNK_SIZE = 65500  # not 65535, leave some space for header
TI_VAR_NAME_LEN = 8


def chunk_firmware(data: bytes, chunk_size: int = CHUNK_SIZE) -> List[bytes]:
    return [data[i : i + chunk_size] for i in range(0, len(data), chunk_size)]


def index_to_suffix(index: int) -> str:
    return chr(ord("A") + index)


def make_var_name(prefix: str, firmware_id: str, suffix: str) -> str:
    name = f"{prefix}{firmware_id}{suffix}"
    if len(name) != TI_VAR_NAME_LEN:
        raise ValueError(
            f"AppVar name '{name}' must be exactly {TI_VAR_NAME_LEN} chars "
            f"(WLFW + 3-letter ID + 1-letter index)."
        )
    return name


def main():
    parser = argparse.ArgumentParser(
        description="Split firmware into 64KB TI-83+/84+CE AppVars using tivars library"
    )
    parser.add_argument(
        "--firmware-name", required=True, help="Firmware base name (e.g., 'htc_9271')"
    )
    parser.add_argument(
        "--manifest",
        default="firmwares/manifest.json",
        help="Path to manifest.json mapping firmware names to hex IDs",
    )
    parser.add_argument(
        "--outdir",
        default="firmwares",
        help="Directory to write .8xv files (default: firmwares)",
    )
    parser.add_argument(
        "--prefix", default="WLFW", help="AppVar name prefix (default: WLFW)"
    )
    parser.add_argument(
        "--comment",
        default="WLNDRVCE Firmware Chunk",
        help="Comment for the AppVar header",
    )
    parser.add_argument(
        "--chunk-size", type=int, default=CHUNK_SIZE, help="Chunk size (default: 65500)"
    )
    args = parser.parse_args()

    if not os.path.isfile(args.manifest):
        print(f"Manifest not found: {args.manifest}", file=sys.stderr)
        sys.exit(1)

    with open(args.manifest, "r", encoding="utf-8") as mf:
        manifest = json.load(mf)

    if args.firmware_name not in manifest:
        print(f"Firmware '{args.firmware_name}' not in manifest", file=sys.stderr)
        sys.exit(1)

    firmware_id_letters = manifest[args.firmware_name].upper()

    if (
        len(firmware_id_letters) != 3
        or not firmware_id_letters.isalpha()
        or not firmware_id_letters.isupper()
    ):
        print(
            f"Invalid 3-letter ID '{firmware_id_letters}' for firmware '{args.firmware_name}'",
            file=sys.stderr,
        )
        sys.exit(1)

    firmware_id = firmware_id_letters

    firmware_bin = None
    for ext in [".bin", ".fw"]:
        candidate = os.path.join("firmwares", "bins", f"{args.firmware_name}{ext}")
        if os.path.isfile(candidate):
            firmware_bin = candidate
            break
    if firmware_bin is None:
        print(
            f"Firmware not found: {os.path.join('firmwares', 'bins', f'{args.firmware_name}.bin')} or .fw",
            file=sys.stderr,
        )
        sys.exit(1)

    os.makedirs(args.outdir, exist_ok=True)

    with open(firmware_bin, "rb") as f:
        blob = f.read()

    chunks = chunk_firmware(blob, args.chunk_size)
    if not chunks:
        print("Firmware file is empty; no appvars produced.", file=sys.stderr)
        sys.exit(1)

    header = TIHeader(comment=args.comment)

    for i, chunk in enumerate(chunks):
        if i >= 26:
            print(
                "Warning: Max chunk count (26) reached. Truncating remaining firmware.",
                file=sys.stderr,
            )
            break

        suffix = index_to_suffix(i)
        var_name = make_var_name(args.prefix, firmware_id, suffix)

        try:
            entry = TIAppVar(name=var_name, data=chunk, archived=True)
            packaged = entry.export(header=header)
            out_name = f"{var_name}.8xv"
            out_path = os.path.join(args.outdir, out_name)
            packaged.save(out_path)
            print(f"Wrote {out_path} ({len(chunk)} bytes payload)")
        except Exception as e:
            print(f"Error creating AppVar {var_name}: {e}", file=sys.stderr)
            sys.exit(1)
    print(f"Done. Created {len(chunks)} AppVars in {args.outdir}")


if __name__ == "__main__":
    main()
