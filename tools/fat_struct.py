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

import os
import sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from dataclasses import dataclass
from io import BufferedRandom
from array import array
import tools.fat_consts as consts
import struct

@dataclass
class FATBootHeader:
	jmpBoot: bytes  # 3 bytes
	OEMName: bytes  # 8 bytes
	bytesPerSec: int
	secPerClus: int
	rsvdSecCnt: int
	numFATs: int
	rootEntCnt: int
	totSec16: int
	mediaType: int
	FATSz16: int
	secPerTrk: int
	numHeads: int
	hiddSec: int
	totSec32: int

	STRUCT_FORMAT = "<3s8sHBHBHHBHHHII"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FATBootHeader":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.jmpBoot,
			self.OEMName,
			self.bytesPerSec,
			self.secPerClus,
			self.rsvdSecCnt,
			self.numFATs,
			self.rootEntCnt,
			self.totSec16,
			self.mediaType,
			self.FATSz16,
			self.secPerTrk,
			self.numHeads,
			self.hiddSec,
			self.totSec32
		)

@dataclass
class FAT16ExtendedHeader:
	FATSz16: int
	drvNum: int
	reserved1: int
	bootSig: int
	volID: int
	volLab: bytes  # 11 bytes
	filSysType: bytes  # 8 bytes

	STRUCT_FORMAT = "<HBBB I 11s8s"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FAT16ExtendedHeader":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.FATSz16,
			self.drvNum,
			self.reserved1,
			self.bootSig,
			self.volID,
			self.volLab,
			self.filSysType
		)

@dataclass
class FAT12ExtendedHeader:
	FATSz16: int
	drvNum: int
	reserved1: int
	bootSig: int
	volID: int
	volLab: bytes  # 11 bytes
	filSysType: bytes  # 8 bytes

	STRUCT_FORMAT = "<HBBB I 11s8s"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FAT12ExtendedHeader":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.FATSz16,
			self.drvNum,
			self.reserved1,
			self.bootSig,
			self.volID,
			self.volLab,
			self.filSysType
		)

@dataclass
class FAT32ExtendedHeader:
	FATSz32: int
	extFlags: int
	FSVer: int
	rootClus: int
	FSInfo: int
	BkBootSec: int
	reserved: bytes  # 12 bytes
	drvNum: int
	reserved1: int
	bootSig: int
	volID: int
	volLab: bytes  # 11 bytes
	filSysType: bytes  # 8 bytes

	STRUCT_FORMAT = "<IHHIHH12sBBB I 11s8s"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FAT32ExtendedHeader":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.FATSz32,
			self.extFlags,
			self.FSVer,
			self.rootClus,
			self.FSInfo,
			self.BkBootSec,
			self.reserved,
			self.drvNum,
			self.reserved1,
			self.bootSig,
			self.volID,
			self.volLab,
			self.filSysType
		)

@dataclass
class FATDirectoryEntry:
	name: bytes  # 11 bytes
	attr: int
	NTRes: int
	crtTimeTenth: int
	crtTime: int
	crtDate: int
	lstAccDate: int
	fstClusHI: int
	wrtTime: int
	wrtDate: int
	fstClusLO: int
	fileSize: int

	STRUCT_FORMAT = "<11sBBBHHHHHHHI"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FATDirectoryEntry":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.name,
			self.attr,
			self.NTRes,
			self.crtTimeTenth,
			self.crtTime,
			self.crtDate,
			self.lstAccDate,
			self.fstClusHI,
			self.wrtTime,
			self.wrtDate,
			self.fstClusLO,
			self.fileSize
		)

	def get_cluster(self) -> int:
		return (self.fstClusHI << 16) | self.fstClusLO

	def set_cluster(self, cluster: int) -> None:
		self.fstClusHI = (cluster >> 16) & 0xFFFF
		self.fstClusLO = cluster & 0xFFFF

	def is_empty(self) -> bool:
		return self.name[0] == 0x00

	def is_deleted(self) -> bool:
		return self.name[0] == 0xE5

	def is_free_slot(self) -> bool:
		return self.is_empty() or self.is_deleted()

	def is_lfn(self) -> bool:
		return (self.attr & consts.LONG_NAME_MASK) == consts.LONG_NAME

	def is_directory(self) -> bool:
		return bool(self.attr & consts.DIRECTORY)

	def __str__(self):
		name_str = self.name.decode('latin1', errors='ignore')
		attr_str = []
		if self.attr & consts.READ_ONLY: attr_str.append("RONLY")
		if self.attr & consts.HIDDEN: attr_str.append("Hidden")
		if self.attr & consts.SYSTEM: attr_str.append("System")
		if self.attr & consts.VOLUME: attr_str.append("Volume")
		if self.attr & consts.DIRECTORY: attr_str.append("Directory")
		if self.attr & consts.ARCHIVE: attr_str.append("Archive")
		
		attrs = " | ".join(attr_str) if attr_str else "None"
		clus = self.get_cluster()
		return f"Entry(name='{name_str}', attr={attrs}, cluster={clus}, size={self.fileSize})"

@dataclass
class FAT32FSInfo:
	leadSignature: int       # 0x41615252
	reserved1: bytes         # 480 bytes
	structSignature: int     # 0x61417272
	freeClusterCount: int    # 0xFFFFFFFF if unknown
	nextFreeCluster: int     # 0xFFFFFFFF if unknown
	reserved2: bytes         # 12 bytes
	trailSignature: int      # 0xAA550000

	STRUCT_FORMAT = "<I480sIII12sI"

	@classmethod
	def from_bytes(cls, data: bytes) -> "FAT32FSInfo":
		unpacked = struct.unpack(cls.STRUCT_FORMAT, data[:struct.calcsize(cls.STRUCT_FORMAT)])
		return cls(*unpacked)

	def to_bytes(self) -> bytes:
		return struct.pack(
			self.STRUCT_FORMAT,
			self.leadSignature,
			self.reserved1,
			self.structSignature,
			self.freeClusterCount,
			self.nextFreeCluster,
			self.reserved2,
			self.trailSignature
		)

	def is_valid(self) -> bool:
		return (
			self.leadSignature == 0x41615252 and
			self.structSignature == 0x61417272 and
			self.trailSignature == 0xAA550000
		)

	@classmethod
	def create_default(cls, free_count: int = 0xFFFFFFFF, next_free: int = 3) -> "FAT32FSInfo":
		return cls(
			leadSignature=0x41615252,
			reserved1=b"\x00" * 480,
			structSignature=0x61417272,
			freeClusterCount=free_count,
			nextFreeCluster=next_free,
			reserved2=b"\x00" * 12,
			trailSignature=0xAA550000
		)

class FAT:
	from tools.mbr import MBREntry
	header_boot: FATBootHeader
	header_extended: FAT16ExtendedHeader | FAT12ExtendedHeader | FAT32ExtendedHeader

	mbr: MBREntry

	dev: BufferedRandom
	table: array | bytearray

	totalClusters: int
	firstDataSector: int
	clusterSize: int

	def lba_to_sector(self, lba: int) -> int:
		return (self.mbr.startLBA + lba) * self.header_boot.bytesPerSec

	def cluster_to_lba(self, cluster: int) -> int:
		return self.firstDataSector + ((cluster - 2) * self.header_boot.secPerClus)

	def __init__(self, header_boot: FATBootHeader, dev: BufferedRandom, mbr: MBREntry, table: array | bytearray, totalClusters: int, firstDataSector: int, clusterSize: int):
		self.header_boot = header_boot
		self.dev = dev
		self.mbr = mbr
		self.table = table
		self.totalClusters = totalClusters
		self.firstDataSector = firstDataSector
		self.clusterSize = clusterSize

	@staticmethod
	def mkfs(dev: BufferedRandom | str, partition: int, start_lba: int, end_lba: int):
		raise NotImplementedError

	def update(self) -> None:
		raise NotImplementedError

	def next_cluster(self, current: int) -> int:
		raise NotImplementedError

	def free_cluster(self) -> int:
		raise NotImplementedError

	def append_cluster(self, cluster: int) -> int:
		raise NotImplementedError

	def free_chain(self, start: int) -> int:
		raise NotImplementedError

	def is_eof(self, cluster: int) -> bool:
		raise NotImplementedError

	def get_eof(self) -> int:
		raise NotImplementedError

	def set_clus(self, cluster: int, value: int) -> None:
		raise NotImplementedError