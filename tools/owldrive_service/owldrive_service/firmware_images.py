from __future__ import annotations

import json
import base64
import struct
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional


GITHUB_FLASH_API = "https://api.github.com/repos/owlRobotics-GmbH/owlRobotPlatform/contents/tools/flash?ref=main"
RP2040_FLASH_BASE = 0x10000000
UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30


@dataclass(frozen=True)
class FirmwareImage:
    id: str
    name: str
    source: str
    size: int
    url: Optional[str] = None
    path: Optional[Path] = None


def list_local_images(repo_root: Path) -> list[FirmwareImage]:
    flash_dir = repo_root / "flash"
    if not flash_dir.exists():
        return []
    images = []
    for path in sorted(flash_dir.glob("*.uf2")):
        images.append(FirmwareImage(id=f"local:{path.name}", name=path.name, source="local", size=path.stat().st_size, path=path))
    return images


def list_github_images() -> list[FirmwareImage]:
    with urllib.request.urlopen(GITHUB_FLASH_API, timeout=10) as response:
        payload = json.loads(response.read().decode("utf-8"))
    images = []
    for item in payload:
        name = item.get("name", "")
        if not name.endswith(".uf2"):
            continue
        images.append(
            FirmwareImage(
                id=f"github:{name}",
                name=name,
                source="github",
                size=int(item.get("size") or 0),
                url=item.get("url"),
            )
        )
    return images


def download_url(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=30) as response:
        data = response.read()
    if "api.github.com/repos/" in url and "/contents/" in url:
        payload = json.loads(data.decode("utf-8"))
        if payload.get("encoding") != "base64":
            raise ValueError("unsupported GitHub content encoding")
        return base64.b64decode(payload.get("content", ""))
    return data


def is_uf2(data: bytes) -> bool:
    if len(data) < 512:
        return False
    start0, start1 = struct.unpack_from("<II", data, 0)
    end_magic = struct.unpack_from("<I", data, 512 - 4)[0]
    return start0 == UF2_MAGIC_START0 and start1 == UF2_MAGIC_START1 and end_magic == UF2_MAGIC_END


def uf2_to_binary(data: bytes) -> bytes:
    if len(data) % 512 != 0:
        raise ValueError("UF2 size is not a multiple of 512 bytes")

    chunks: dict[int, bytes] = {}
    min_addr: Optional[int] = None
    max_addr = 0

    for block_offset in range(0, len(data), 512):
        block = data[block_offset : block_offset + 512]
        start0, start1, _flags, target_addr, payload_size, _block_no, _num_blocks, _family = struct.unpack_from("<IIIIIIII", block, 0)
        end_magic = struct.unpack_from("<I", block, 508)[0]
        if start0 != UF2_MAGIC_START0 or start1 != UF2_MAGIC_START1 or end_magic != UF2_MAGIC_END:
            raise ValueError(f"invalid UF2 block at offset {block_offset}")
        if payload_size <= 0 or payload_size > 476:
            raise ValueError(f"invalid UF2 payload size {payload_size} at offset {block_offset}")
        if target_addr < RP2040_FLASH_BASE:
            continue
        payload = bytes(block[32 : 32 + payload_size])
        chunks[target_addr] = payload
        min_addr = target_addr if min_addr is None else min(min_addr, target_addr)
        max_addr = max(max_addr, target_addr + payload_size)

    if min_addr is None:
        raise ValueError("UF2 contains no RP2040 flash payload")
    if min_addr != RP2040_FLASH_BASE:
        raise ValueError(f"UF2 starts at 0x{min_addr:08x}, expected 0x{RP2040_FLASH_BASE:08x}")

    image = bytearray(b"\xFF" * (max_addr - min_addr))
    for addr, payload in chunks.items():
        offset = addr - min_addr
        image[offset : offset + len(payload)] = payload
    return bytes(image).rstrip(b"\xFF")


def firmware_payload(data: bytes, filename: str = "") -> bytes:
    if filename.lower().endswith(".uf2") or is_uf2(data):
        return uf2_to_binary(data)
    return data
