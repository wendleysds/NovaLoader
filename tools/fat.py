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
import argparse
import array

import tools.fat_struct as fat_struct
import tools.fat_consts as fat_consts
from tools.fat_struct import FAT
from tools.fat_impl import FAT12, FAT16, FAT32
from tools.mbr import MBREntry

class FATFD:
	entry: fat_struct.FATDirectoryEntry
	firstCluster: int
	currentCluster: int
	dirCluster: int
	cursor: int

	def __init__(self, entry: fat_struct.FATDirectoryEntry, dirCluster: int):
		cluster = entry.get_cluster()
		self.entry = entry
		self.firstCluster = cluster
		self.currentCluster = cluster
		self.cursor = 0
		self.dirCluster = dirCluster

def fat_format_name(filename: str) -> str:
	if not filename or len(filename) == 0:
		return '\0'

	if filename == '.':
		return '.          '
	if filename == '..':
		return '..         '

	dot = filename.rfind('.')
	if dot != -1:
		name_part = filename[:dot]
		ext_part = filename[dot+1:]
	else:
		name_part = filename
		ext_part = ''

	name_part = name_part.replace('.', '').replace(' ', '')
	ext_part = ext_part.replace('.', '').replace(' ', '')

	if len(name_part) > 8:
		name_part = name_part[:6] + '~1'

	if len(ext_part) > 3:
		ext_part = ext_part[:3]

	return name_part.ljust(8).upper() + ext_part.ljust(3).upper()


class FATFS:
	fat: FAT

	def __init__(self, device: BufferedRandom | str, mbr: MBREntry | None = None, partition: int = 1, fsInfoStartCluster: int = 3):
		device = open(device, 'rb+') if isinstance(device, str) else device

		if mbr is None:
			device.seek(0)
			sec0 = device.read(512)
			if len(sec0) == 512 and sec0[510:512] == b'\x55\xAA':
				mbrs = [MBREntry.from_bytes(sec0[0x1BE + i * 16 : 0x1BE + (i + 1) * 16]) for i in range(4)]
				part_idx = (partition - 1) if 1 <= partition <= 4 else 0
				if mbrs[part_idx].startLBA > 0 and mbrs[part_idx].sizeInLBA > 0:
					device.seek(mbrs[part_idx].startLBA * 512)
					psec = device.read(512)
					if len(psec) == 512 and psec[510:512] == b'\x55\xAA':
						mbr = mbrs[part_idx]

			if mbr is None:
				mbr = MBREntry(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

		device.seek(mbr.startLBA * 512)
		content = device.read(512)

		if len(content) < 512 or content[510:512] != b'\x55\xAA':
			device.close()
			raise ValueError("Invalid FAT boot sector signature (0x55AA missing)!")

		bpb = fat_struct.FATBootHeader.from_bytes(content[0:36])
		fat32ext = fat_struct.FAT32ExtendedHeader.from_bytes(content[36:90])
		fat16ext = fat_struct.FAT16ExtendedHeader.from_bytes(content[36:64])
		fat12ext = fat_struct.FAT12ExtendedHeader.from_bytes(content[36:64])

		fatSize = bpb.FATSz16 if bpb.FATSz16 != 0 else fat32ext.FATSz32
		totSec = bpb.totSec16 if bpb.totSec16 != 0 else bpb.totSec32

		rootDirSectors = ((bpb.rootEntCnt * 32) + (bpb.bytesPerSec - 1)) // bpb.bytesPerSec
		firstDataSector = bpb.rsvdSecCnt + (bpb.numFATs * fatSize) + rootDirSectors

		dataSectors = totSec - firstDataSector
		countOfClusters = dataSectors // bpb.secPerClus if bpb.secPerClus > 0 else 0

		isFat32 = bpb.FATSz16 == 0 or countOfClusters >= 65525
		isFat16 = not isFat32 and countOfClusters >= 4085
		isFat12 = not isFat32 and countOfClusters < 4085

		fatStartSector = bpb.rsvdSecCnt
		fatBytes = fatSize * bpb.bytesPerSec

		device.seek((mbr.startLBA + fatStartSector) * bpb.bytesPerSec)

		if isFat32:
			table = array.array("I")
			table.frombytes(device.read(fatBytes))

			device.seek((mbr.startLBA + fat32ext.FSInfo) * bpb.bytesPerSec)
			fsInfoContent = device.read(512)

			if len(fsInfoContent) == 512:
				fsInfo = fat_struct.FAT32FSInfo.from_bytes(fsInfoContent)
			else:
				fsInfo = fat_struct.FAT32FSInfo.create_default()

			if not fsInfo.is_valid():
				fsInfo = fat_struct.FAT32FSInfo.create_default(
					free_count=countOfClusters - 1,
					next_free=3
				)
			else:
				if fsInfo.freeClusterCount == 0xFFFFFFFF:
					fsInfo.freeClusterCount = int(countOfClusters - 1)
				if fsInfo.nextFreeCluster == 0xFFFFFFFF:
					fsInfo.nextFreeCluster = int(fsInfoStartCluster)

			self.fat = FAT32(
				bpb, device, mbr, table,
				int(countOfClusters), int(firstDataSector),
				int(bpb.bytesPerSec * bpb.secPerClus),
				fsInfo, fat32ext
			)

		elif isFat16:
			table = array.array("H")
			table.frombytes(device.read(fatBytes))

			self.fat = FAT16(
				bpb, device, mbr, table,
				int(countOfClusters), int(firstDataSector),
				int(bpb.bytesPerSec * bpb.secPerClus),
				fat16ext
			)

		elif isFat12:
			raw_fat = device.read(fatBytes)
			table = bytearray(raw_fat)

			self.fat = FAT12(
				bpb, device, mbr, table,
				int(countOfClusters), int(firstDataSector),
				int(bpb.bytesPerSec * bpb.secPerClus),
				fat12ext
			)
		else:
			device.close()
			raise ValueError("Invalid FAT volume configuration!")

	@staticmethod
	def format_file(device: BufferedRandom | str, start_lba: int, end_lba: int, partition: int = 1):
		part_size = end_lba - start_lba
		if part_size < 4085:
			FAT12.mkfs(device, partition, start_lba, end_lba)
		elif part_size < 65525:
			FAT16.mkfs(device, partition, start_lba, end_lba)
		else:
			FAT32.mkfs(device, partition, start_lba, end_lba)

	def cluster_to_lba(self, cluster: int) -> int:
		return self.fat.firstDataSector + ((cluster - 2) * self.fat.header_boot.secPerClus)

	def _root_dir_start_sector(self) -> int:
		return self.fat.header_boot.rsvdSecCnt + (self.fat.header_boot.numFATs * self.fat.header_boot.FATSz16)

	def _is_root_dir_cluster(self, dirCluster: int) -> bool:
		if isinstance(self.fat.header_extended, fat_struct.FAT32ExtendedHeader):
			return False
		return dirCluster in (fat_consts.ROOT_DIRECTORY, 0xFFFF, 0xFFFFFFFF, -1) or (dirCluster & 0xFFFF) == 0xFFFF

	def _get_directory_sectors(self, dirCluster: int):
		if self._is_root_dir_cluster(dirCluster):
			root_start = self._root_dir_start_sector()
			total_sectors = (self.fat.header_boot.rootEntCnt * 32) // self.fat.header_boot.bytesPerSec
			entries_per_sec = self.fat.header_boot.bytesPerSec // 32

			for sector in range(root_start, root_start + total_sectors):
				yield (sector, entries_per_sec)
		else:
			cluster = dirCluster
			sec_per_clus = self.fat.header_boot.secPerClus
			entries_per_sec = self.fat.header_boot.bytesPerSec // 32

			while True:
				lba = self.cluster_to_lba(cluster)
				for s_off in range(sec_per_clus):
					yield (lba + s_off, entries_per_sec)

				cluster = self.fat.next_cluster(cluster)
				if self.fat.is_eof(cluster):
					break

	def _iter_dir_entries(self, dirCluster: int):
		f = self.fat.dev

		for sector, entries_count in self._get_directory_sectors(dirCluster):
			sec_byte_offset = self.fat.lba_to_sector(sector)
			f.seek(sec_byte_offset)

			for entry_idx in range(entries_count):
				data = f.read(32)
				if len(data) < 32:
					return

				entry = fat_struct.FATDirectoryEntry.from_bytes(data)

				if entry.is_empty():
					return

				if not entry.is_deleted() and not entry.is_lfn():
					yield entry

	def ls(self, dirCluster: int) -> list[fat_struct.FATDirectoryEntry]:
		return list(self._iter_dir_entries(dirCluster))

	def search(self, dirCluster: int, name: str, formatName: bool = True) -> fat_struct.FATDirectoryEntry | None:
		formatted_name = fat_format_name(name) if formatName else name.ljust(11).upper()

		for entry in self._iter_dir_entries(dirCluster):
			entry_name = entry.name.decode('latin1', errors='ignore')
			if entry_name == formatted_name:
				return entry

		return None

	def create(self, dirCluster: int, name: str, attr: int) -> int:
		formatted_name = fat_format_name(name)

		# Do not allow duplicate entries.
		if self.search(dirCluster, formatted_name, formatName=False) is not None:
			return -2

		is_dir = bool(attr & fat_consts.DIRECTORY)

		# Allocate the first cluster only for directories.
		new_cluster = 0

		if is_dir:
			new_cluster = self._alloc_cluster()
			if new_cluster < 0:
				return -1

		# Find a free directory entry slot.
		slot = self._find_free_directory_slot(dirCluster)

		if slot is None:
			# Directory could not be expanded.
			if new_cluster:
				self.fat.set_clus(new_cluster, fat_consts.UNUSED)

			return -1

		sector, entry_idx = slot

		# Create and write the directory entry.
		entry = self._make_directory_entry(
			formatted_name,
			attr,
			new_cluster,
		)

		self._write_directory_entry(
			sector,
			entry_idx,
			entry,
		)

		# Initialize "." and ".." for directories.
		if is_dir:
			self._init_subdirectory(new_cluster, dirCluster)

		self.fat.update()

		return new_cluster


	def _alloc_cluster(self) -> int:
		cluster = self.fat.free_cluster()

		if cluster < 2 or self.fat.is_eof(cluster):
			return -1

		self.fat.set_clus(cluster, self.fat.get_eof())

		return cluster


	def _find_free_directory_slot(
		self,
		dirCluster: int,
	) -> tuple[int, int] | None:

		f = self.fat.dev

		# First search for an existing free entry.
		for sector, entries_count in self._get_directory_sectors(dirCluster):
			sector_offset = self.fat.lba_to_sector(sector)
			f.seek(sector_offset)

			for entry_idx in range(entries_count):
				data = f.read(32)

				if len(data) < 32:
					break

				entry = fat_struct.FATDirectoryEntry.from_bytes(data)

				if entry.is_free_slot():
					return sector, entry_idx

		# No free slot. Try to expand the directory.
		if self._is_root_dir_cluster(dirCluster):
			return None

		last_cluster = self._directory_last_cluster(dirCluster)

		expanded_cluster = self.fat.append_cluster(last_cluster)

		if expanded_cluster < 0:
			return None

		# A newly allocated directory cluster must be empty.
		self._zero_cluster(expanded_cluster)

		return self.cluster_to_lba(expanded_cluster), 0


	def _directory_last_cluster(self, dirCluster: int) -> int:
		cluster = dirCluster

		while True:
			next_cluster = self.fat.next_cluster(cluster)

			if self.fat.is_eof(next_cluster):
				return cluster

			cluster = next_cluster


	def _zero_cluster(self, cluster: int) -> None:
		f = self.fat.dev

		lba = self.cluster_to_lba(cluster)
		offset = self.fat.lba_to_sector(lba)

		f.seek(offset)
		f.write(b'\x00' * self.fat.clusterSize)


	def _make_directory_entry(
		self,
		name: str,
		attr: int,
		cluster: int,
	) -> fat_struct.FATDirectoryEntry:

		return fat_struct.FATDirectoryEntry(
			name=name.encode('latin1'),
			attr=attr,
			NTRes=0,
			crtTimeTenth=0,
			crtTime=0,
			crtDate=0x5461,
			lstAccDate=0x5461,
			fstClusHI=(cluster >> 16) & 0xFFFF,
			wrtTime=0,
			wrtDate=0x5461,
			fstClusLO=cluster & 0xFFFF,
			fileSize=0,
		)


	def _write_directory_entry(
		self,
		sector: int,
		entry_idx: int,
		entry: fat_struct.FATDirectoryEntry,
	) -> None:

		f = self.fat.dev

		offset = (
			self.fat.lba_to_sector(sector)
			+ entry_idx * 32
		)

		f.seek(offset)
		f.write(entry.to_bytes())


	def _init_subdirectory(
		self,
		cluster: int,
		parent_cluster: int,
	) -> None:
		f = self.fat.dev

		lba = self.cluster_to_lba(cluster)
		offset = self.fat.lba_to_sector(lba)

		f.seek(offset)

		# clear dir
		f.write(b'\x00' * self.fat.clusterSize)

		f.seek(offset)

		# "."
		dot = self._make_directory_entry(
			'.',
			fat_consts.DIRECTORY,
			cluster,
		)

		# ".."
		parent = (
			parent_cluster
			if not self._is_root_dir_cluster(parent_cluster)
			else 0
		)

		dotdot = self._make_directory_entry(
			'..',
			fat_consts.DIRECTORY,
			parent,
		)

		f.write(dot.to_bytes())
		f.write(dotdot.to_bytes())


	def remove(self, dirCluster: int, name: str) -> int:
		formatted_name = fat_format_name(name)
		f = self.fat.dev

		for sector, entries_count in self._get_directory_sectors(dirCluster):
			sec_byte_offset = self.fat.lba_to_sector(sector)
			f.seek(sec_byte_offset)

			for entry_idx in range(entries_count):
				offset = sec_byte_offset + (entry_idx * 32)
				f.seek(offset)
				data = f.read(32)
				if len(data) < 32:
					break

				entry = fat_struct.FATDirectoryEntry.from_bytes(data)
				if entry.is_empty():
					return -1

				if entry.is_deleted() or entry.is_lfn():
					continue

				if entry.name.decode('latin1', errors='ignore') == formatted_name:
					# Mark entry deleted
					deleted_name = bytearray(entry.name)
					deleted_name[0] = 0xE5
					entry.name = bytes(deleted_name)

					f.seek(offset)
					f.write(entry.to_bytes())

					# Free cluster chain
					start_clus = entry.get_cluster()
					if start_clus >= 2:
						self.fat.free_chain(start_clus)

					self.fat.update()
					return 0

		return -1

	def walk(self, path: str) -> fat_struct.FATDirectoryEntry | None:
		parts = [p for p in path.strip('/').split('/') if p]

		if isinstance(self.fat.header_extended, fat_struct.FAT32ExtendedHeader):
			root_cluster = self.fat.header_extended.rootClus
		else:
			root_cluster = fat_consts.ROOT_DIRECTORY

		if not parts:
			root = fat_struct.FATDirectoryEntry(
				name=b'ROOT       ',
				attr=fat_consts.DIRECTORY | fat_consts.SYSTEM,
				NTRes=0, crtTimeTenth=0, crtTime=0, crtDate=0,
				lstAccDate=0, fstClusHI=0, wrtTime=0, wrtDate=0,
				fstClusLO=0, fileSize=0
			)
			if root_cluster != fat_consts.ROOT_DIRECTORY:
				root.set_cluster(root_cluster)
			else:
				root.set_cluster(fat_consts.ROOT_DIRECTORY)
			return root

		curr_cluster = root_cluster
		target_entry = None

		for part in parts:
			target_entry = self.search(curr_cluster, part)
			if not target_entry:
				return None

			if not target_entry.is_directory() and part != parts[-1]:
				return None

			curr_cluster = target_entry.get_cluster()

		return target_entry

	def _update_entry_on_disk(self, dirCluster: int, entry: fat_struct.FATDirectoryEntry):
		f = self.fat.dev
		formatted_name = entry.name.decode('latin1', errors='ignore')

		for sector, entries_count in self._get_directory_sectors(dirCluster):
			sec_byte_offset = self.fat.lba_to_sector(sector)
			f.seek(sec_byte_offset)

			for entry_idx in range(entries_count):
				offset = sec_byte_offset + (entry_idx * 32)
				f.seek(offset)
				data = f.read(32)
				if len(data) < 32:
					break

				curr_entry = fat_struct.FATDirectoryEntry.from_bytes(data)
				if curr_entry.name.decode('latin1', errors='ignore') == formatted_name:
					f.seek(offset)
					f.write(entry.to_bytes())
					return True

		return False

	def open(self, path: str) -> FATFD | None:
		path = path.rstrip('/')
		tokens = path.split('/')
		filename = tokens[-1]
		parent_path = '/'.join(tokens[:-1]) if len(tokens) > 1 else '/'
		if not parent_path:
			parent_path = '/'

		parent_entry = self.walk(parent_path)
		if not parent_entry:
			return None

		dir_cluster = parent_entry.get_cluster()
		if parent_path == '/' and not isinstance(self.fat.header_extended, fat_struct.FAT32ExtendedHeader):
			dir_cluster = fat_consts.ROOT_DIRECTORY

		file_entry = self.search(dir_cluster, filename)
		if not file_entry:
			return None

		return FATFD(file_entry, dir_cluster)

	def seek(self, fd: FATFD, offset: int, whence: int = 0) -> int:
		filesize = fd.entry.fileSize
		if whence == 0:
			target = offset
		elif whence == 1:
			target = fd.cursor + offset
		elif whence == 2:
			target = filesize + offset
		else:
			return -1

		if target < 0 or target > filesize:
			return -1

		clusterOffset = target // self.fat.clusterSize
		cluster = fd.firstCluster

		for _ in range(clusterOffset):
			cluster = self.fat.next_cluster(cluster)
			if self.fat.is_eof(cluster):
				return -1

		fd.currentCluster = cluster
		fd.cursor = target
		return target

	def read(self, fd: FATFD, count: int) -> bytes:
		if fd.cursor >= fd.entry.fileSize or count <= 0:
			return b''

		remaining = min(count, fd.entry.fileSize - fd.cursor)
		content = bytearray()
		f = self.fat.dev

		while remaining > 0 and fd.currentCluster >= 2 and not self.fat.is_eof(fd.currentCluster):
			clusterOffset = fd.cursor % self.fat.clusterSize
			bytesLeftInCluster = self.fat.clusterSize - clusterOffset

			toRead = min(remaining, bytesLeftInCluster)
			lba = self.cluster_to_lba(fd.currentCluster)
			byte_offset = self.fat.lba_to_sector(lba) + clusterOffset

			f.seek(byte_offset)
			data = f.read(toRead)
			read_len = len(data)

			if read_len == 0:
				break

			content.extend(data)
			fd.cursor += read_len
			remaining -= read_len

			if fd.cursor % self.fat.clusterSize == 0 and remaining > 0:
				next_clus = self.fat.next_cluster(fd.currentCluster)
				if self.fat.is_eof(next_clus):
					break
				fd.currentCluster = next_clus

		return bytes(content)

	def write(self, fd: FATFD, buffer: str | bytes, count: int) -> int:
		if isinstance(buffer, str):
			buffer = buffer.encode('latin1')

		buffer = buffer[:count]
		remaining = len(buffer)
		totalWritten = 0
		f = self.fat.dev

		# Allocate initial cluster if file is empty
		if fd.firstCluster < 2:
			new_cluster = self.fat.free_cluster()
			if new_cluster == -1 or self.fat.is_eof(new_cluster):
				return -1

			self.fat.set_clus(new_cluster, self.fat.get_eof())
			fd.firstCluster = new_cluster
			fd.currentCluster = new_cluster
			fd.entry.set_cluster(new_cluster)

		while remaining > 0:
			clusterOffset = fd.cursor % self.fat.clusterSize
			bytesLeftInCluster = self.fat.clusterSize - clusterOffset
			toWrite = min(remaining, bytesLeftInCluster)

			lba = self.cluster_to_lba(fd.currentCluster)
			byte_offset = self.fat.lba_to_sector(lba) + clusterOffset

			f.seek(byte_offset)
			written = f.write(buffer[totalWritten:totalWritten + toWrite])

			fd.cursor += written
			totalWritten += written
			remaining -= written

			if fd.cursor % self.fat.clusterSize == 0 and remaining > 0:
				nextCluster = self.fat.next_cluster(fd.currentCluster)
				if self.fat.is_eof(nextCluster):
					nextCluster = self.fat.append_cluster(fd.currentCluster)
					if nextCluster < 0:
						break

				fd.currentCluster = nextCluster

		if fd.cursor > fd.entry.fileSize:
			fd.entry.fileSize = fd.cursor

		# Write updated directory entry back to disk
		self._update_entry_on_disk(fd.dirCluster, fd.entry)
		self.fat.update()

		return totalWritten

def main():
	parser = argparse.ArgumentParser(description="FAT Filesystem Management Tool")
	subparsers = parser.add_subparsers(dest="command", help="Available subcommands")

	# Subcommand: format
	parser_fmt = subparsers.add_parser("format", help="Format volume with FAT12/16/32")
	parser_fmt.add_argument("device", type=str, help="Disk image or device path")
	parser_fmt.add_argument("--start-lba", type=int, default=0, help="Partition start LBA")
	parser_fmt.add_argument("--end-lba", type=int, required=True, help="Partition end LBA")
	parser_fmt.add_argument("--partition", type=int, default=1, help="Partition number (1-4)")

	# Subcommand: ls
	parser_ls = subparsers.add_parser("ls", help="List directory contents")
	parser_ls.add_argument("device", type=str, help="Disk image path")
	parser_ls.add_argument("path", type=str, nargs="?", default="/", help="Directory path")
	parser_ls.add_argument("-p", "--partition", type=int, default=1, help="Partition number (1-4)")

	# Subcommand: cat
	parser_cat = subparsers.add_parser("cat", help="Display file content")
	parser_cat.add_argument("device", type=str, help="Disk image path")
	parser_cat.add_argument("path", type=str, help="File path in volume")
	parser_cat.add_argument("-p", "--partition", type=int, default=1, help="Partition number (1-4)")

	# Subcommand: put
	parser_put = subparsers.add_parser("put", help="Write local file to FAT volume")
	parser_put.add_argument("device", type=str, help="Disk image path")
	parser_put.add_argument("local_file", type=str, help="Local host file path")
	parser_put.add_argument("target_path", type=str, help="Target FAT file path")
	parser_put.add_argument("-p", "--partition", type=int, default=1, help="Partition number (1-4)")

	# Subcommand: mkdir
	parser_mkdir = subparsers.add_parser("mkdir", help="Create directory")
	parser_mkdir.add_argument("device", type=str, help="Disk image path")
	parser_mkdir.add_argument("path", type=str, help="Directory path to create")
	parser_mkdir.add_argument("-p", "--partition", type=int, default=1, help="Partition number (1-4)")

	# Subcommand: rm
	parser_rm = subparsers.add_parser("rm", help="Remove file from FAT volume")
	parser_rm.add_argument("device", type=str, help="Disk image path")
	parser_rm.add_argument("path", type=str, help="File path to delete")
	parser_rm.add_argument("-p", "--partition", type=int, default=1, help="Partition number (1-4)")

	# Subcommand: info
	parser_info = subparsers.add_parser("info", help="Display volume information")
	parser_info.add_argument("device", type=str, help="Disk image path")
	parser_info.add_argument("-p", "--partition", type=int, default=1, help="Partition number (1-4)")

	# Subcommand: stat
	parser_stat = subparsers.add_parser("stat", help="Show file statistics")
	parser_stat.add_argument("device", type=str, help="Disk image path")
	parser_stat.add_argument("path", type=str, help="File path to stat")
	parser_stat.add_argument("-p", "--partition", type=int, default=1, help="Partition number (1-4)")

	args = parser.parse_args()

	if not args.command:
		parser.print_help()
		return 1

	if args.command == "format":
		print(f"Formatting {args.device} (LBA {args.start_lba} to {args.end_lba}, Partition {args.partition})...")
		FATFS.format_file(args.device, args.start_lba, args.end_lba, args.partition)
		print("Format completed successfully.")
		return 0

	partition = getattr(args, "partition", 1)
	fatfs = FATFS(args.device, partition=partition)

	if args.command == "info":
		fat = fatfs.fat
		bpb = fat.header_boot
		ext = fat.header_extended
		print(f"OEM Name:       {bpb.OEMName.decode('latin1', errors='ignore')}")
		print(f"Bytes/Sector:   {bpb.bytesPerSec}")
		print(f"Sectors/Clus:   {bpb.secPerClus}")
		print(f"Cluster Size:   {fat.clusterSize} bytes")
		print(f"Total Clusters: {fat.totalClusters}")
		print(f"FileSystem:     {ext.filSysType.decode('latin1', errors='ignore').strip()}")
		if hasattr(fat, 'fsInfo'):
			print(f"FSInfo Free:    {fat.fsInfo.freeClusterCount}")
			print(f"FSInfo NextFree:{fat.fsInfo.nextFreeCluster}")
		return 0

	if args.command == "ls":
		entry = fatfs.walk(args.path)
		if not entry or not entry.is_directory():
			print(f"error: directory '{args.path}' not found")
			return 1

		clus = entry.get_cluster()
		if args.path == '/' and not isinstance(fatfs.fat.header_extended, fat_struct.FAT32ExtendedHeader):
			clus = fat_consts.ROOT_DIRECTORY

		entries = fatfs.ls(clus)
		for e in entries:
			kind = "<DIR> " if e.is_directory() else "      "
			name = e.name.decode('latin1', errors='ignore')
			print(f"{kind} {name}  {e.fileSize} bytes")
		return 0

	if args.command == "cat":
		fd = fatfs.open(args.path)
		if not fd:
			print(f"error: file '{args.path}' not found")
			return 1

		content = fatfs.read(fd, fd.entry.fileSize)
		sys.stdout.buffer.write(content)
		return 0

	if args.command == "put":
		if not os.path.exists(args.local_file):
			print(f"error: local file '{args.local_file}' not found")
			return 1

		with open(args.local_file, "rb") as lf:
			data = lf.read()

		tokens = args.target_path.rstrip('/').split('/')
		filename = tokens[-1]
		parent_path = '/'.join(tokens[:-1]) if len(tokens) > 1 else '/'
		if not parent_path: parent_path = '/'

		parent_entry = fatfs.walk(parent_path)
		if not parent_entry or not parent_entry.is_directory():
			print(f"error: parent directory '{parent_path}' not found")
			return 1

		p_clus = parent_entry.get_cluster()
		if parent_path == '/' and not isinstance(fatfs.fat.header_extended, fat_struct.FAT32ExtendedHeader):
			p_clus = fat_consts.ROOT_DIRECTORY

		fd = fatfs.open(args.target_path)
		if not fd:
			fatfs.create(p_clus, filename, fat_consts.ARCHIVE)
			fd = fatfs.open(args.target_path)

		if not fd:
			print(f"error: failed to create '{args.target_path}'")
			return 1

		written = fatfs.write(fd, data, len(data))
		print(f"Wrote {written} bytes to {args.target_path}")
		return 0

	if args.command == "mkdir":
		tokens = args.path.rstrip('/').split('/')
		dirname = tokens[-1]
		parent_path = '/'.join(tokens[:-1]) if len(tokens) > 1 else '/'
		if not parent_path: parent_path = '/'

		parent_entry = fatfs.walk(parent_path)
		if not parent_entry or not parent_entry.is_directory():
			print(f"error: parent directory '{parent_path}' not found")
			return 1

		p_clus = parent_entry.get_cluster()
		if parent_path == '/' and not isinstance(fatfs.fat.header_extended, fat_struct.FAT32ExtendedHeader):
			p_clus = fat_consts.ROOT_DIRECTORY

		res = fatfs.create(p_clus, dirname, fat_consts.DIRECTORY)
		if res < 0:
			print(f"error: failed to create directory '{args.path}' (code {res})")
			return 1
		print(f"Directory '{args.path}' created successfully.")
		return 0

	if args.command == "rm":
		tokens = args.path.rstrip('/').split('/')
		filename = tokens[-1]
		parent_path = '/'.join(tokens[:-1]) if len(tokens) > 1 else '/'
		if not parent_path: parent_path = '/'

		parent_entry = fatfs.walk(parent_path)
		if not parent_entry or not parent_entry.is_directory():
			print(f"error: parent directory '{parent_path}' not found")
			return 1

		p_clus = parent_entry.get_cluster()
		if parent_path == '/' and not isinstance(fatfs.fat.header_extended, fat_struct.FAT32ExtendedHeader):
			p_clus = fat_consts.ROOT_DIRECTORY

		res = fatfs.remove(p_clus, filename)
		if res < 0:
			print(f"error: failed to remove '{args.path}'")
			return 1
		print(f"Removed '{args.path}'.")
		return 0

	if args.command == "stat":
		entry = fatfs.walk(args.path)
		if not entry:
			print(f"error: file or directory '{args.path}' not found")
			return 1
		print(entry)

		cluster_chain = []
		cur = entry.get_cluster()
		while 0x2 <= cur < fatfs.fat.get_eof():
			cluster_chain.append(cur)
			cur = fatfs.fat.next_cluster(cur)

		lbas = []
		for cluster in cluster_chain:
			lbas.append(fatfs.cluster_to_lba(cluster))

		print("clusters: ", cluster_chain)
		print("lbas: ", [lba + fatfs.fat.mbr.startLBA for lba in lbas])
		return 0

if __name__ == "__main__":
	sys.exit(main())
