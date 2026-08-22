"""
 * SPDX-License-Identifier: MIT
 *
 * NovaLoader - Lightweight, modular, and configurable x86 bootloader for BIOS/MBR
 *
 * Copyright (c) 2026 Wendley Santos
 *
 * This file is part of NovaLoader.
 * See the LICENSE file in the project root for license information.
"""

import struct
from dataclasses import dataclass

@dataclass
class MBREntry:
	bootInd: int

	startHead: int
	startSec: int
	startCyl: int

	partType: int

	endHead: int
	endSec: int
	endCyl: int

	startLBA: int
	sizeInLBA: int

	STRUCT_FORMAT = "<BBBBBBBBII"

	@classmethod
	def from_bytes(cls, data: bytes) -> "MBREntry":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)
	
	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.bootInd,
			self.startHead,
			self.startSec,
			self.startCyl,
			self.partType,
			self.endHead,
			self.endSec,
			self.endCyl,
			self.startLBA,
			self.sizeInLBA	
		)