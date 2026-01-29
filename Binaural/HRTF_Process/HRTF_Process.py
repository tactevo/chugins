import os
import scipy.io.wavfile as wav
import numpy as np
import re

def readHRTF(filedestination):
	files = sorted(os.listdir(filedestination))
	irl = []
	irr = []
	ele = []
	azi = []
	curPath = os.path.dirname(os.path.abspath(__file__))
	for file in files:
		fs, data = wav.read(curPath+'/'+filedestination+'/'+file)
		azimuth = int(re.search('(?<=azi_)(.*?)(?=_)', file).group())
		elevation = int(re.search('(?<=ele_)(.*?)(?=_)', file).group())
		if fs == 44.1e3:
			data = data/(2**15)
		if fs == 48e3 or fs == 96e3:
			data = data/(2**31)
		irl.append(data[:, 0])
		irr.append(data[:, 1])
		ele.append(elevation)
		azi.append(azimuth)
	irl = np.array(irl)
	irr = np.array(irr)
	ele = np.array(ele)
	azi = np.array(azi)
	# Sort everything by azimuth and then elevation
	aInd = np.argsort(azi)
	aUniq = np.unique(azi)
	azi = azi[aInd]
	irl = irl[aInd, :]
	irr = irr[aInd, :]
	ele = ele[aInd]
	for i in range(len(aUniq)):
		aW = np.where(azi == aUniq[i])
		eInd = np.argsort(ele[aW]) + np.min(aW)
		ele[aW] = ele[eInd]
		irl[aW, :] = irl[eInd, :]
		irr[aW, :] = irr[eInd, :]
	outStr = 'HRTF3_{:d}K'.format(int(fs/1000))
	genCPPArr('test.txt', outStr+'_L', np.array(irl), 'float')
	genCPPArr('test.txt', outStr + '_R', np.array(irr), 'float', 'a')
	genCPPArr('test.txt', outStr + '_azi', azi, 'int', 'a')
	genCPPArr('test.txt', outStr + '_ele', ele, 'int', 'a')

def matchHRTF(filedestination, aziele, write=False, classname=None, filename=None):
	files = sorted(os.listdir(filedestination))
	irl = []
	irr = []
	ele = []
	azi = []
	curPath = os.path.dirname(os.path.abspath(__file__))
	for a, e in aziele:
		for file in files:
			fs, data = wav.read(curPath + '/' + filedestination + '/' + file)
			azimuth = int(re.search('(?<=azi_)(.*?)(?=_)', file).group())
			elevation = int(re.search('(?<=ele_)(.*?)(?=_)', file).group())
			if azimuth == a and elevation == e:
				if fs == 44.1e3:
					data = data / (2 ** 15)
				if fs == 48e3 or fs == 96e3:
					data = data / (2 ** 31)
				irl.append(data[:, 0])
				irr.append(data[:, 1])
				ele.append(elevation)
				azi.append(azimuth)
	irl = np.array(irl)
	irr = np.array(irr)
	ele = np.array(ele)
	azi = np.array(azi)
	if np.all( ele == np.array(aziele)[:,1]) and np.all(azi == np.array(aziele)[:,0]):
		if write:
			genCPPArr(filename, classname + '_L', np.array(irl), 'static const float')
			genCPPArr(filename, classname + '_R', np.array(irr), 'static const float', 'a')
		return irl, irr
	else:
		return 0

def genCPPMap(fname, cname, arr, mode='w'):
	f = open(fname, mode)
	f.write('std::map<int, int> {} = {{'.format(cname))
	for i in range(len(arr)):
		f.write("{{ {:d}, {:d} }}".format(arr[i], i))
		if i < len(arr)-1:
			f.write(',')
		else:
			f.write('};')
	f.write('\n')
	f.close()

def genCPPArr(fname, cname, array, dtype, mode='w'):
	f = open(fname, mode) # Open the file
	shape = array.shape
	itemStr = '{} {}'.format(dtype, cname)
	itemSize = ''
	for dim in array.shape:
		itemSize = itemSize + '[{:d}]'.format(dim)
	startStr = itemStr + itemSize + ' = {\n'
	f.write(startStr)
	# Maybe there's a recursive way to do this
	if len(array.shape) == 2:
		for i in range(array.shape[0]):
			if (dtype == 'int'):
				x_arrstr = np.char.mod('%d', array[i, :].astype(int))
			elif (dtype == 'float' or dtype == 'static const float'):
				x_arrstr = np.char.mod('%1.20f', array[i, :])
			x_str = ', '.join(x_arrstr)
			f.write('\t{' + x_str)
			if i < array.shape[0]-1:
				f.write('},\n')
			else:
				f.write('}};')
	else:
		if (dtype=='int'):
			x_arrstr = np.char.mod('%d', array.astype(int))
		elif(dtype=='float' or dtype=='static const float'):
			x_arrstr = np.char.mod('%1.20f', array)
		x_str = ','.join(x_arrstr)
		f.write('\t' + x_str + '};')
	f.write('\n')
	f.close()

if __name__=='__main__':
	aziele = [ [0, 51], [90, 51], [180, 51], [270, 51], [45, 14], [135, 14], [225, 14], [315, 14], [0, -14], [90, -14], [180, -14], [270, -14], [45, -51], [135, -51], [225, -51], [315, -51]]
	matchHRTF('Subject_001_Wav/DFC/44K_16bit', aziele, True, 'Binaural::HRTF44K', 'HRTF44K.txt')
	matchHRTF('Subject_001_Wav/DFC/48K_24bit', aziele, True, 'Binaural::HRTF48K', 'HRTF48K.txt')
	matchHRTF('Subject_001_Wav/DFC/96K_24bit', aziele, True, 'Binaural::HRTF96K', 'HRTF96K.txt')