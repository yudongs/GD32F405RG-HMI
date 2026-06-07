"""MKLink dump_memory protocol — high-throughput multi-region memory read.

Format (sent to MCU as one line):
    dump_memory <addr1> <size1> [<addr2> <size2> ...]

MCU responds with a binary stream of frames. Each frame is:
    [magic(4)] [frame_id(2)] [chunk_id(2)] [chunk_ready(2)] [lines_per_chunk(2)]
    [total_chunks(2)] [pixel_data_size(2)] [export_y_start(2)] [reserved(2)]
    [chunk_data(N bytes)]

This module is a stub — the full streaming parser will be added when
needed (e.g., for LCD capture). For now, only the command builder is
provided, since the basic read_ram path is sufficient for the SDK.
"""

import struct

DUMP_MEM_MAGIC = 0x444D444D  # "DMDM"
DUMP_MEM_HEADER_SIZE = 20  # 10 x uint16


def build_dump_mem_command(regions, delay_ms: float = 0.0005) -> str:
    """Build a `dump_memory` text command from a list of (addr, size) pairs."""
    parts = ["dump_memory"]
    for addr, size in regions:
        parts.append(f"0x{addr:08X}")
        parts.append(str(size))
    if delay_ms:
        parts.append(f"delay={delay_ms}")
    return " ".join(parts)


class DumpMemoryParser:
    """Parser for the dump_memory binary stream protocol.

    Stateful: feed bytes via `feed()` and get frames via iteration.
    Not used in the current SDK (basic read_ram is enough); included
    for future use (LCD capture, large data transfers).
    """

    def __init__(self, region_sizes):
        self.region_sizes = region_sizes
        self._buf = bytearray()
        self._header_size = DUMP_MEM_HEADER_SIZE
        self._frame_size = DUMP_MEM_HEADER_SIZE + sum(region_sizes)

    def feed(self, data: bytes):
        """Feed bytes; yields complete frames as they arrive."""
        self._buf.extend(data)
        while len(self._buf) >= self._frame_size:
            frame = bytes(self._buf[:self._frame_size])
            del self._buf[:self._frame_size]
            yield self._parse_frame(frame)

    def _parse_frame(self, data: bytes) -> dict:
        if len(data) < self._header_size:
            return {"magic": 0, "regions": []}
        magic, frame_id, chunk_id, chunk_ready, lines_per_chunk, total_chunks, pixel_data_size, export_y_start, _, _ = struct.unpack_from("<10H", data, 0)
        regions = []
        offset = self._header_size
        for size in self.region_sizes:
            regions.append((0, data[offset:offset + size]))
            offset += size
        return {
            "magic": magic,
            "frame_id": frame_id,
            "chunk_id": chunk_id,
            "chunk_ready": chunk_ready,
            "regions": regions,
        }
