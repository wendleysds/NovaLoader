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
from io import BufferedRandom
from array import array
import struct

from tools.mbr import MBREntry
import tools.fat_struct as fat_struct
import tools.fat_consts as fat_consts
from tools.fat_struct import FAT

class FAT32(FAT):
	fsInfo: fat_struct.FAT32FSInfo

	def __init__(
			self, header_boot: fat_struct.FATBootHeader, dev: BufferedRandom, mbr: MBREntry, 
			table: array, totalClusters: int, firstDataSector: int, clusterSize: int,
			fsinfo: fat_struct.FAT32FSInfo, header_extended: fat_struct.FAT32ExtendedHeader
		):
		super().__init__(header_boot, dev, mbr, table, totalClusters, firstDataSector, clusterSize)
		self.fsInfo = fsinfo
		self.header_extended = header_extended

	@staticmethod
	def mkfs(dev: BufferedRandom | str, partition: int, start_lba: int, end_lba: int):
		f = open(dev, 'rb+') if isinstance(dev, str) else dev

		part_idx = (partition - 1) if partition >= 1 else 0
		part_size = end_lba - start_lba

		if part_size < 2048:
			raise ValueError("Partition size is too small for FAT32")

		f.seek(0)
		sec0 = f.read(512)
		if len(sec0) < 512:
			sec0 = bytearray(512)

		mbrs = [MBREntry.from_bytes(sec0[0x1BE + i*16:(0x1BE + (i+1)*16)]) for i in range(4)]

		if sec0[510:512] != b'\x55\xAA':
			sec0 = bytearray(sec0)
			f.seek(0)
			mbr = MBREntry(
				bootInd=0, startHead=0, startSec=0, startCyl=0,
				partType=0x0C, endHead=0, endSec=0, endCyl=0,
				startLBA=start_lba, sizeInLBA=part_size
			)
			sec0[446:446+16] = mbr.to_bytes()
			sec0[510:512] = b'\x55\xAA'
			f.write(sec0)
			mbrs[0] = mbr
			part_idx = 0
		elif mbrs[part_idx].partType == 0 or mbrs[part_idx].startLBA == 0:
			mbrs[part_idx] = MBREntry(
				bootInd=0, startHead=0, startSec=0, startCyl=0,
				partType=0x0C, endHead=0, endSec=0, endCyl=0,
				startLBA=start_lba, sizeInLBA=part_size
			)
			f.seek(446 + part_idx * 16)
			f.write(mbrs[part_idx].to_bytes())

		mbr = mbrs[part_idx]

		bytesPerSec = 512
		secPerClus = 8
		if part_size > 67108864:    # > 32GB
			secPerClus = 64
		elif part_size > 33554432:  # > 16GB
			secPerClus = 32
		elif part_size > 16777216:  # > 8GB
			secPerClus = 16

		# Ensure total clusters >= 65525 for valid FAT32 cluster count
		while (part_size // secPerClus) < 65525 and secPerClus > 1:
			secPerClus //= 2

		rsvdSecCnt = 32
		numFATs = 2

		hboot = fat_struct.FATBootHeader(
			jmpBoot=b'\xEB\x58\x90',
			OEMName=b'MSWIN4.1',
			bytesPerSec=bytesPerSec,
			secPerClus=secPerClus,
			rsvdSecCnt=rsvdSecCnt,
			numFATs=numFATs,
			rootEntCnt=0,
			totSec16=0,
			mediaType=0xF8,
			FATSz16=0,
			secPerTrk=63,
			numHeads=16,
			hiddSec=mbr.startLBA,
			totSec32=mbr.sizeInLBA
		)

		hextended = fat_struct.FAT32ExtendedHeader(
			FATSz32=0,
			extFlags=0,
			FSVer=0,
			rootClus=2,
			FSInfo=1,
			BkBootSec=6,
			reserved=b'\x00' * 12,
			drvNum=0x80,
			reserved1=0,
			bootSig=0x29,
			volID=0xCAFEBAB1,
			volLab=b'NOVA    VOL',
			filSysType=b'FAT32   '
		)

		tmpVal1 = mbr.sizeInLBA - hboot.rsvdSecCnt
		tmpVal2 = (128 * hboot.secPerClus) + hboot.numFATs
		fatSz32 = (tmpVal1 + (tmpVal2 - 1)) // tmpVal2
		hextended.FATSz32 = fatSz32

		fatStartSector = hboot.rsvdSecCnt
		fatBytes = fatSz32 * bytesPerSec

		firstDataSector = hboot.rsvdSecCnt + (hboot.numFATs * fatSz32)
		dataSectors = mbr.sizeInLBA - firstDataSector
		totalClusters = dataSectors // hboot.secPerClus

		table = bytearray(fatBytes)
		table[0:4]  = b'\xF8\xFF\xFF\x0F'  # cluster 0: media descriptor
		table[4:8]  = b'\xFF\xFF\xFF\x0F'  # cluster 1: EOF marker
		table[8:12] = b'\xFF\xFF\xFF\x0F'  # cluster 2 (root): EOF marker

		def _SEC(sector):
			return (sector + mbr.startLBA) * bytesPerSec

		# 1. Write Boot Sector at sector 0
		f.seek(_SEC(0))
		boot_sector = bytearray(512)
		boot_sector[0:90] = hboot.to_bytes() + hextended.to_bytes()
		boot_sector[510:512] = b'\x55\xAA'
		f.write(boot_sector)

		# 2. Write FSInfo Sector at sector 1
		fsinfo = fat_struct.FAT32FSInfo.create_default(
			free_count=totalClusters - 1,
			next_free=3
		)
		f.seek(_SEC(hextended.FSInfo))
		f.write(fsinfo.to_bytes())

		# 3. Write Backup Boot Sector at sector 6
		f.seek(_SEC(hextended.BkBootSec))
		f.write(boot_sector)

		# 4. Write FAT1 and FAT2
		f.seek(_SEC(fatStartSector))
		f.write(table)
		f.seek(_SEC(fatStartSector + fatSz32))
		f.write(table)

		# 5. Zero out Root Cluster (Cluster 2)
		root_cluster_offset = (firstDataSector + (hextended.rootClus - 2) * secPerClus)
		f.seek(_SEC(root_cluster_offset))
		f.write(b'\x00' * (secPerClus * bytesPerSec))
		f.flush()

	def update(self):
		fatStartSector = self.header_boot.rsvdSecCnt
		fatSize = self.header_extended.FATSz32
		f = self.dev

		# Update FSInfo if present
		if self.fsInfo.is_valid():
			f.seek(self.lba_to_sector(self.header_extended.FSInfo))
			f.write(self.fsInfo.to_bytes())

		packed_table = struct.pack("<" + "I" * len(self.table), *self.table)
		for i in range(self.header_boot.numFATs):
			f.seek(self.lba_to_sector(fatStartSector + i * fatSize))
			f.write(packed_table)
		f.flush()

	def is_eof(self, cluster: int) -> bool:
		return (cluster & 0x0FFFFFFF) >= 0x0FFFFFF8

	def next_cluster(self, current: int) -> int:
		if current < 0 or current >= len(self.table):
			return 0x0FFFFFFF
		return self.table[current] & 0x0FFFFFFF

	def get_eof(self) -> int:
		return 0x0FFFFFFF

	def set_clus(self, cluster: int, value: int):
		if 0 <= cluster < len(self.table):
			self.table[cluster] = (self.table[cluster] & 0xF0000000) | (value & 0x0FFFFFFF)

	def free_cluster(self) -> int:
		start = self.fsInfo.nextFreeCluster if (3 <= self.fsInfo.nextFreeCluster < self.totalClusters) else 3
		for cluster in range(start, self.totalClusters):
			if (self.table[cluster] & 0x0FFFFFFF) == fat_consts.UNUSED:
				return cluster

		for cluster in range(3, start):
			if (self.table[cluster] & 0x0FFFFFFF) == fat_consts.UNUSED:
				return cluster

		return 0x0FFFFFFF

	def append_cluster(self, cluster: int) -> int:
		foundCluster = self.free_cluster()

		if foundCluster == -1 or self.is_eof(foundCluster):
			return -1

		self.set_clus(cluster, foundCluster)
		self.set_clus(foundCluster, self.get_eof())

		if self.fsInfo.nextFreeCluster != 0xFFFFFFFF:
			self.fsInfo.nextFreeCluster = foundCluster + 1
		if self.fsInfo.freeClusterCount != 0xFFFFFFFF and self.fsInfo.freeClusterCount > 0:
			self.fsInfo.freeClusterCount -= 1

		return foundCluster

	def free_chain(self, start: int) -> int:
		count = 0
		cur = start

		while 2 <= cur < self.totalClusters and not self.is_eof(cur):
			if cur >= len(self.table):
				break

			next_clus = self.next_cluster(cur)
			self.set_clus(cur, fat_consts.UNUSED)
			cur = next_clus
			count += 1

		if self.fsInfo.freeClusterCount != 0xFFFFFFFF:
			self.fsInfo.freeClusterCount += count
		if self.fsInfo.nextFreeCluster != 0xFFFFFFFF:
			self.fsInfo.nextFreeCluster = start

		return count


class FAT16(FAT):
	header_extended: fat_struct.FAT16ExtendedHeader

	def __init__(
			self, header_boot: fat_struct.FATBootHeader, dev: BufferedRandom, mbr: MBREntry, 
			table: array, totalClusters: int, firstDataSector: int, clusterSize: int,
			header_extended: fat_struct.FAT16ExtendedHeader):
		super().__init__(header_boot, dev, mbr, table, totalClusters, firstDataSector, clusterSize)
		self.header_extended = header_extended

	@staticmethod
	def mkfs(dev: BufferedRandom | str, partition: int, start_lba: int, end_lba: int):
		f = open(dev, 'rb+') if isinstance(dev, str) else dev

		part_idx = (partition - 1) if partition >= 1 else 0
		part_size = end_lba - start_lba

		if part_size < 64:
			raise ValueError("Partition size is too small for FAT16")

		f.seek(0)
		sec0 = f.read(512)
		if len(sec0) < 512:
			sec0 = bytearray(512)

		mbrs = [MBREntry.from_bytes(sec0[0x1BE + i*16:(0x1BE + (i+1)*16)]) for i in range(4)]

		if sec0[510:512] != b'\x55\xAA':
			sec0 = bytearray(sec0)
			f.seek(0)
			mbr = MBREntry(
				bootInd=0, startHead=0, startSec=0, startCyl=0,
				partType=0x06, endHead=0, endSec=0, endCyl=0,
				startLBA=start_lba, sizeInLBA=part_size
			)
			sec0[446:446+16] = mbr.to_bytes()
			sec0[510:512] = b'\x55\xAA'
			f.write(sec0)
			mbrs[0] = mbr
			part_idx = 0
		elif mbrs[part_idx].partType == 0 or mbrs[part_idx].startLBA == 0:
			mbrs[part_idx] = MBREntry(
				bootInd=0, startHead=0, startSec=0, startCyl=0,
				partType=0x06, endHead=0, endSec=0, endCyl=0,
				startLBA=start_lba, sizeInLBA=part_size
			)
			f.seek(446 + part_idx * 16)
			f.write(mbrs[part_idx].to_bytes())

		mbr = mbrs[part_idx]

		bytesPerSec = 512
		rsvdSecCnt = 1
		numFATs = 2
		rootEntCnt = 512

		if part_size < 32680:
			secPerClus = 2
		elif part_size < 262144:
			secPerClus = 4
		elif part_size < 524288:
			secPerClus = 8
		else:
			secPerClus = 16

		rootDirSectors = ((rootEntCnt * 32) + (bytesPerSec - 1)) // bytesPerSec
		tmpVal1 = mbr.sizeInLBA - (rsvdSecCnt + rootDirSectors)
		tmpVal2 = (256 * secPerClus) + numFATs
		fatSz16 = (tmpVal1 + (tmpVal2 - 1)) // tmpVal2

		totSec16 = part_size if part_size < 65536 else 0
		totSec32 = part_size if part_size >= 65536 else 0

		hboot = fat_struct.FATBootHeader(
			jmpBoot=b'\xEB\x3C\x90',
			OEMName=b'MSWIN4.1',
			bytesPerSec=bytesPerSec,
			secPerClus=secPerClus,
			rsvdSecCnt=rsvdSecCnt,
			numFATs=numFATs,
			rootEntCnt=rootEntCnt,
			totSec16=totSec16,
			mediaType=0xF8,
			FATSz16=fatSz16,
			secPerTrk=63,
			numHeads=16,
			hiddSec=mbr.startLBA,
			totSec32=totSec32
		)

		hextended = fat_struct.FAT16ExtendedHeader(
			FATSz16=fatSz16,
			drvNum=0x80,
			reserved1=0,
			bootSig=0x29,
			volID=0xCAFEBAB1,
			volLab=b'NOVA    VOL',
			filSysType=b'FAT16   '
		)

		fatStartSector = hboot.rsvdSecCnt
		fatBytes = fatSz16 * bytesPerSec

		table = bytearray(fatBytes)
		table[0:2] = b'\xF8\xFF'  # cluster 0: media descriptor
		table[2:4] = b'\xFF\xFF'  # cluster 1: EOF marker

		def _SEC(sector):
			return (sector + mbr.startLBA) * bytesPerSec

		# Write Boot Sector
		f.seek(_SEC(0))
		boot_sector = bytearray(512)
		boot_sector[0:62] = hboot.to_bytes() + hextended.to_bytes()
		boot_sector[510:512] = b'\x55\xAA'
		f.write(boot_sector)

		# Write FAT1 and FAT2
		f.seek(_SEC(fatStartSector))
		f.write(table)
		f.seek(_SEC(fatStartSector + fatSz16))
		f.write(table)

		# Zero out Root Directory sectors
		rootDirStartSector = fatStartSector + (numFATs * fatSz16)
		f.seek(_SEC(rootDirStartSector))
		f.write(b'\x00' * (rootDirSectors * bytesPerSec))
		f.flush()

	def update(self):
		fatStartSector = self.header_boot.rsvdSecCnt
		fatSize = self.header_boot.FATSz16
		f = self.dev

		packed_table = struct.pack("<" + "H" * len(self.table), *self.table)
		for i in range(self.header_boot.numFATs):
			f.seek(self.lba_to_sector(fatStartSector + i * fatSize))
			f.write(packed_table)
		f.flush()

	def is_eof(self, cluster: int) -> bool:
		return (cluster & 0xFFFF) >= 0xFFF8

	def next_cluster(self, current: int) -> int:
		if current < 0 or current >= len(self.table):
			return 0xFFFF
		return self.table[current] & 0xFFFF

	def get_eof(self) -> int:
		return 0xFFFF

	def set_clus(self, cluster: int, value: int):
		if 0 <= cluster < len(self.table):
			self.table[cluster] = value & 0xFFFF

	def free_cluster(self) -> int:
		for cluster in range(2, self.totalClusters):
			if (self.table[cluster] & 0xFFFF) == fat_consts.UNUSED:
				return cluster
		return 0xFFFF

	def append_cluster(self, cluster: int) -> int:
		foundCluster = self.free_cluster()

		if foundCluster == -1 or self.is_eof(foundCluster):
			return -1

		self.set_clus(cluster, foundCluster)
		self.set_clus(foundCluster, self.get_eof())
		return foundCluster

	def free_chain(self, start: int) -> int:
		count = 0
		cur = start

		while 2 <= cur < self.totalClusters and not self.is_eof(cur):
			if cur >= len(self.table):
				break

			next_clus = self.next_cluster(cur)
			self.set_clus(cur, fat_consts.UNUSED)
			cur = next_clus
			count += 1

		return count


class FAT12(FAT):
	header_extended: fat_struct.FAT12ExtendedHeader

	def __init__(
			self, header_boot: fat_struct.FATBootHeader, dev: BufferedRandom, mbr: MBREntry, 
			table: bytearray | array, totalClusters: int, firstDataSector: int, clusterSize: int,
			header_extended: fat_struct.FAT12ExtendedHeader):
		super().__init__(header_boot, dev, mbr, table, totalClusters, firstDataSector, clusterSize)
		self.header_extended = header_extended

	@staticmethod
	def mkfs(dev: BufferedRandom | str, partition: int, start_lba: int, end_lba: int):
		f = open(dev, 'rb+') if isinstance(dev, str) else dev

		part_idx = (partition - 1) if partition >= 1 else 0
		part_size = end_lba - start_lba

		if part_size < 16:
			raise ValueError("Partition size is too small for FAT12")

		f.seek(0)
		sec0 = f.read(512)
		if len(sec0) < 512:
			sec0 = bytearray(512)

		mbrs = [MBREntry.from_bytes(sec0[0x1BE + i*16:(0x1BE + (i+1)*16)]) for i in range(4)]

		if sec0[510:512] != b'\x55\xAA':
			sec0 = bytearray(sec0)
			f.seek(0)
			mbr = MBREntry(
				bootInd=0, startHead=0, startSec=0, startCyl=0,
				partType=0x01, endHead=0, endSec=0, endCyl=0,
				startLBA=start_lba, sizeInLBA=part_size
			)
			sec0[446:446+16] = mbr.to_bytes()
			sec0[510:512] = b'\x55\xAA'
			f.write(sec0)
			mbrs[0] = mbr
			part_idx = 0
		elif mbrs[part_idx].partType == 0 or mbrs[part_idx].startLBA == 0:
			mbrs[part_idx] = MBREntry(
				bootInd=0, startHead=0, startSec=0, startCyl=0,
				partType=0x01, endHead=0, endSec=0, endCyl=0,
				startLBA=start_lba, sizeInLBA=part_size
			)
			f.seek(446 + part_idx * 16)
			f.write(mbrs[part_idx].to_bytes())

		mbr = mbrs[part_idx]

		bytesPerSec = 512
		rsvdSecCnt = 1
		numFATs = 2
		rootEntCnt = 224

		if part_size < 2048:
			secPerClus = 1
		elif part_size < 4096:
			secPerClus = 2
		elif part_size < 8192:
			secPerClus = 4
		else:
			secPerClus = 8

		totSec16 = part_size if part_size < 65536 else 0
		totSec32 = 0

		rootDirSectors = ((rootEntCnt * 32) + (bytesPerSec - 1)) // bytesPerSec
		tmpVal1 = mbr.sizeInLBA - (rsvdSecCnt + rootDirSectors)
		tmpVal2 = ((256 * secPerClus) + numFATs) * 3 // 4
		fatSz16 = (tmpVal1 + (tmpVal2 - 1)) // tmpVal2

		hboot = fat_struct.FATBootHeader(
			jmpBoot=b'\xEB\x3C\x90',
			OEMName=b'MSWIN4.1',
			bytesPerSec=bytesPerSec,
			secPerClus=secPerClus,
			rsvdSecCnt=rsvdSecCnt,
			numFATs=numFATs,
			rootEntCnt=rootEntCnt,
			totSec16=totSec16,
			mediaType=0xF8,
			FATSz16=fatSz16,
			secPerTrk=63,
			numHeads=16,
			hiddSec=mbr.startLBA,
			totSec32=totSec32
		)

		hextended = fat_struct.FAT12ExtendedHeader(
			FATSz16=fatSz16,
			drvNum=0x80,
			reserved1=0,
			bootSig=0x29,
			volID=0xCAFEBAB1,
			volLab=b'NOVA    VOL',
			filSysType=b'FAT12   '
		)

		fatStartSector = hboot.rsvdSecCnt
		fatBytes = fatSz16 * bytesPerSec

		table = bytearray(fatBytes)
		table[0:3] = b'\xF8\xFF\xFF'  # cluster 0: F8 F, cluster 1: FF F

		def _SEC(sector):
			return (sector + mbr.startLBA) * bytesPerSec

		# Write Boot Sector
		f.seek(_SEC(0))
		boot_sector = bytearray(512)
		boot_sector[0:62] = hboot.to_bytes() + hextended.to_bytes()
		boot_sector[510:512] = b'\x55\xAA'
		f.write(boot_sector)

		# Write FAT1 and FAT2
		f.seek(_SEC(fatStartSector))
		f.write(table)
		f.seek(_SEC(fatStartSector + fatSz16))
		f.write(table)

		# Zero out Root Directory sectors
		rootDirStartSector = fatStartSector + (numFATs * fatSz16)
		f.seek(_SEC(rootDirStartSector))
		f.write(b'\x00' * (rootDirSectors * bytesPerSec))
		f.flush()

	def update(self):
		fatStartSector = self.header_boot.rsvdSecCnt
		fatSize = self.header_boot.FATSz16
		f = self.dev

		packed_table = bytes(self.table)
		for i in range(self.header_boot.numFATs):
			f.seek(self.lba_to_sector(fatStartSector + i * fatSize))
			f.write(packed_table)
		f.flush()

	def is_eof(self, cluster: int) -> bool:
		return (cluster & 0x0FFF) >= 0x0FF8

	def next_cluster(self, current: int) -> int:
		offset = (current * 3) // 2
		if offset + 1 >= len(self.table):
			return 0x0FFF

		b0 = self.table[offset]
		b1 = self.table[offset + 1]

		if current & 1:
			next_clus = (b0 >> 4) | (b1 << 4)
		else:
			next_clus = b0 | ((b1 & 0x0F) << 8)

		return next_clus & 0x0FFF

	def get_eof(self) -> int:
		return 0x0FFF

	def set_clus(self, cluster: int, value: int):
		offset = (cluster * 3) // 2
		if offset + 1 >= len(self.table):
			return

		value &= 0x0FFF

		if cluster & 1:
			self.table[offset] = (self.table[offset] & 0x0F) | ((value & 0x0F) << 4)
			self.table[offset + 1] = (value >> 4) & 0xFF
		else:
			self.table[offset] = value & 0xFF
			self.table[offset + 1] = (self.table[offset + 1] & 0xF0) | ((value >> 8) & 0x0F)

	def free_cluster(self) -> int:
		for cluster in range(2, self.totalClusters):
			val = self.next_cluster(cluster)
			if val == fat_consts.UNUSED:
				return cluster

		return 0x0FFF

	def append_cluster(self, cluster: int) -> int:
		foundCluster = self.free_cluster()

		if foundCluster == -1 or self.is_eof(foundCluster):
			return -1

		self.set_clus(cluster, foundCluster)
		self.set_clus(foundCluster, self.get_eof())
		return foundCluster

	def free_chain(self, start: int) -> int:
		count = 0
		cur = start

		while 2 <= cur < self.totalClusters and not self.is_eof(cur):
			offset = (cur * 3) // 2
			if offset + 1 >= len(self.table):
				break

			next_clus = self.next_cluster(cur)
			self.set_clus(cur, fat_consts.UNUSED)
			cur = next_clus
			count += 1

		return count
