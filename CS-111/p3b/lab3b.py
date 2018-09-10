#!/bin/python


import sys
import csv

# function of block_consistency_audit	
def block_consistency_audit(inodes, superblock, groups, fblocks, indirect):
	first_block = groups[0]["indtable"]+((groups[0]["num_ind"]*superblock["inode_size"])/superblock["blk_size"])

	################# INVALID & RESERVED of direct blocks #################
	legal_blocks = []
	#iterate over each inode in the inodes dictionary collected from the .csv file
	for index, inode in inodes.iteritems():
		if inode["type"] != 's':
			for i in xrange(0, 12):
				if inode["blocks"][i] < 0 or inode["blocks"][i] >= superblock["blk_total"]:
					print "INVALID BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][i], index, 0)
				elif inode["blocks"][i] > 0 and inode["blocks"][i] < first_block:
					print "RESERVED BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][i], index, 0)
				elif inode["blocks"][i] > 0:
					#keep track of all the legal blocks used in the file system, this is for allocated block audit
					legal_blocks.append(inode["blocks"][i])

			#special cases for rows of "indirect", "double indirect", and "triple indirect", because of differnet output format
			if inode["blocks"][12] < 0 or inode["blocks"][12] >= superblock["blk_total"]:
				print "INVALID INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][12], index, 12)
			elif inode["blocks"][12] > 0 and inode["blocks"][12] < first_block:
				print "RESERVED INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][12], index, 12)
			elif inode["blocks"][12] > 0:
				legal_blocks.append(inode["blocks"][12])

			if inode["blocks"][13] < 0 or inode["blocks"][13] >= superblock["blk_total"]:
				print "INVALID DOUBLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][13], index, 268)
			elif inode["blocks"][13] > 0 and inode["blocks"][13] < first_block:
				print "RESERVED DOUBLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][13], index, 268)
			elif inode["blocks"][13] > 0:
				legal_blocks.append(inode["blocks"][13])

			if inode["blocks"][14] < 0 or inode["blocks"][14] >= superblock["blk_total"]:
				print "INVALID TRIPLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][14], index, 65804)
			elif inode["blocks"][14] > 0 and inode["blocks"][14] < first_block:
				print "RESERVED TRIPLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" % (inode["blocks"][14], index, 65804)
			elif inode["blocks"][14] > 0:
				legal_blocks.append(inode["blocks"][14])

	############# INVALID and RESERVED in indirect inodes ##############
	for index, indirects in indirect.iteritems():
		for inode in indirects:
			indirect_level = ''
			if inode["level"] == '1':
				indirect_level = 'INDIRECT'
			if inode["level"] == '2':
				indirect_level = 'DOUBLE INDIRECT'
			if inode["level"] == '3':
				indirect_level = 'TRIPLE INDIRECT'

			if inode["referenced_blk"] < 0 or inode["referenced_blk"] >= superblock["blk_total"]:
				print "INVALID %s BLOCK %d IN INODE %d AT OFFSET %d" % (indirect_level, inode["referenced_blk"], index, inode["offset"])
			elif inode["referenced_blk"] > 0 and inode["referenced_blk"] < first_block:
				print "RESERVED %s BLOCK %d ININODE %d AT OFFSET %d" % (indirect_level, inode["referenced_blk"], index, inode["offset"])
			elif inode["referenced_blk"] > 0:
		 		legal_blocks.append(inode["referenced_blk"])

	############ UNREFRENCED and ALLOCATED blocks #############
	for x in range (first_block, groups[0]["num_blk"]):
		#blocks that are neither mentioned in the csv nor in the BFREE list are unrefrenced
		if x not in legal_blocks and x not in fblocks["free_block"]:
			print "UNREFERENCED BLOCK %d" %x
		#blocks that are both mentioned in the csv and in BFREE list are allocated on freelist
		if x in legal_blocks and x in fblocks["free_block"]:
			print "ALLOCATED BLOCK %d ON FREELIST" %x

	########### DUPLICATE blocks ######################
	#temp_block and dump_block are temparary assistance lists to keep track of duplicate blocks
	#temp_block contains all block entries in the legal blocks
	#dump_block contains all block entries that are duplicated
	temp_block = []
	dump_block = []
	for b in legal_blocks:
		if b not in temp_block:
			temp_block.append(b)
		elif b in temp_block and b not in dump_block and b not in fblocks["free_block"]:
			dump_block.append(b)

	#print out duplicate audit
	for dump in dump_block:
		for index, inode in inodes.iteritems():
			if inode["type"] != 's':
				for i in xrange(0, 12):
					if inode["blocks"][i] == dump:
						print "DUPLICATE BLOCK %d IN INODE %d AT OFFSET %d" %(inode["blocks"][i], index, 0)
				if inode["blocks"][12] == dump:
					print "DUPLICATE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(inode["blocks"][12], index, 12)
				if inode["blocks"][13] == dump:
					print "DUPLICATE DOUBLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(inode["blocks"][13], index, 268)
				if inode["blocks"][14] == dump:
					print "DUPLICATE TRIPLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(inode["blocks"][14], index, 65804)

		for index, indirects in indirect.iteritems():
			for inode in indirects:

				indirect_level = ''
				if inode["level"] == '1':
					indirect_level = 'INDIRECT'
				if inode["level"] == '2':
					indirect_level = 'DOUBLE INDIRECT'
				if inode["level"] == '3':
					indirect_level = 'TRIPLE INDIRECT'

				if inode["referenced_blk"] == dump:
					print "DUPLICATE %s BLOCK %d IN NODE %d AT OFFSET %d" % (indirect_level, inode["referenced_blk"], index, inode["offset"])

# function of inode_allocation_audit
def inode_allocation_audit(finodes, superblock, inodes):

	#keep a list of allocated inodes
	allocated_inodes=[]

	######### ALLOCATED INODES ###########
	for index, inode in inodes.iteritems():
		allocated_inodes.append(index)
		if index in finodes["free_inode"]:
			print "ALLOCATED INODE %d ON FREELIST" % index

	######### UNALLOCATED INODES ###############
	#if an inode is neither allocated nor in the IFREE list, it's unallocated/unreferenced
	for i in xrange(superblock["first_inode"], superblock["inode_total"] + 1):
		if i not in finodes["free_inode"] and i not in allocated_inodes:
			print "UNALLOCATED INODE %d NOT ON FREELIST" % i


# function of directory_consistency_audit 
def directory_consistency_audit(inodes, dirent, finodes, superblock):

	######## LINK VS LINKCOUNT ###########
	#report the wrong linkcounts
	for index, inode in inodes.iteritems():
		num_links = 0
		for i in dirent:
			if i["reference"] == index:
				num_links+=1
		if inode["link"] != num_links:
			print "INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" % (index, num_links, inode["link"])

	#for x in xrange(superblock["first_inode"], superblock["inode_total"] + 1):
		#if i not in finodes["free_inode"] and i not in finodes["allocated"]:
			#num_links = 0
			#for i in dirent:
				#if i["reference"] == index:
					#num_links+=1
			#if num_links != 1:
				#print "INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" % (x, num_links, 1)

	#keep a list of allocated inodes for later use
	allocated_inodes=[]
	for index, inode in inodes.iteritems():
		allocated_inodes.append(index)

	########### INVALID and UNALLOCATED inodes poited to ##########
	parents = dict()
	for x in dirent:
		if x["reference"] not in finodes["free_inode"] and x["reference"] not in allocated_inodes:
			print "DIRECTORY INODE %d NAME %s INVALID INODE %d" % (x["parent"], x["name"], x["reference"])
		elif x["reference"] not in allocated_inodes:
			print "DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d" % (x["parent"], x["name"], x["reference"])
		elif x["reference"] not in parents:
			parents[x["reference"]] = x["parent"]

	########## '.' and '..' special cases #########
	#report incorrect parent-child matches
	for x in dirent:
		if x["name"] == "'.'":
			if x["reference"] != x["parent"]:
				print "DIRECTORY INODE %d NAME %s LINK TO INODE %d SHOULD BE %d" % (x["parent"], x["name"], x["reference"], x["parent"])
		if x["name"] == "'..'":
			if x["reference"] != parents[x["parent"]]:
				print "DIRECTORY INODE %d NAME %s LINK TO INODE %d SHOULD BE %d" % (x["parent"], x["name"], x["reference"], parents[x["parent"]])



def main():
	#the program should take in one and only argument
	#which is the name of the .csv file
	if len(sys.argv) != 2:
		sys.stderr.write('Error: Only one argument is allowed.\n')
		exit(1)

	#exception handler to detect is the .csv file is readable
	try:
		file = open(sys.argv[1], 'r')
	except IOError:
		sys.stderr.write('Error: Cannot open input .csv file.\n')
		exit(1)

	#dictionaries used in the program
	superblock = dict()
	groups = dict()
	fblocks = dict()
	finodes = dict()
	inodes = dict()
	indirect =dict()
	#lists used in the program
	dirent = []

	#open the .csv file and collect information
	with open(sys.argv[1], 'r') as file1:
		rows = csv.reader(file1, delimiter = ',')
		for i in rows:
			if (i[0] == "SUPERBLOCK"):
				superblock = dict(blk_total = int(i[1]), inode_total = int(i[2]), blk_size = int(i[3]), inode_size = int(i[4]), first_inode = int(i[7]))
			elif (i[0] == "GROUP"):
				groups[int(i[1])] = dict(num_blk = int(i[2]), num_ind = int(i[3]), free_blk = int(i[4]), free_ind = int(i[5]), blkmap = int(i[6]), indmap = int(i[7]), indtable = int(i[8]))
			elif (i[0] == "BFREE"):
				fblocks.setdefault("free_block", []).append(int(i[1]))
			elif (i[0] == "IFREE"):
				finodes.setdefault("free_inode", []).append(int(i[1]))
			elif (i[0] == "INODE"):
				inodes[int(i[1])] = dict(type = i[2], owner = int(i[4]), group = int(i[5]), link = int(i[6]), filesize = int(i[10]), num_blk = int(i[11]))
				if (i[2] != 's'):
					inodes[int(i[1])]["blocks"] = [int(i[x]) for x in xrange(12, 27)]
			elif (i[0] == "DIRENT"):
				dirent.append(dict(parent = int(i[1]), offset = int(i[2]), reference = int(i[3]), entry_len = int(i[4]), name_len = int(i[5]), name = i[6]))
			elif (i[0] == "INDIRECT"):
				indirect.setdefault(int(i[1]), []).append(dict(level = int(i[2]), offset = int(i[3]), scanned_blk = int(i[4]), referenced_blk = int(i[5])))

	########## AUDIT ############
	block_consistency_audit(inodes, superblock, groups, fblocks, indirect)
	inode_allocation_audit(finodes, superblock, inodes)
	directory_consistency_audit(inodes, dirent, finodes, superblock)
	sys.exit(0)

if __name__ == '__main__':
	main()

