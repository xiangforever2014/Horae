#ifndef _Layer_H
#define _Layer_H
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <cmath>
#include <stdlib.h>
#include <bitset>
#include <memory.h>
#include <algorithm>
#include <sys/time.h>
#include "../HashFunction.h"
#include "../params.h"
using namespace std;

struct basket {
	uint16_t src[SLOTNUM];
	uint16_t dst[SLOTNUM]; 
	weight_type weight[SLOTNUM];
};

struct node {
	uint32_t key;
	weight_type weight;
};

class findx {
public:
	findx(const uint32_t va) { value = va; }
	bool operator()(const vector<node>::value_type &nod) {
		if (nod.key == value)
			return true;
		else
			return false;
	}
private:
	uint32_t value;
};

class findv {
public:
	findv(uint32_t va) { value = va; }
	bool operator()(vector<node> &vc) {
		if (vc[0].key == value)
			return true;
		else
			return false;
	}
private:
	uint32_t value;
};

class Layer {
protected:
	const uint32_t granularity;			// the granularity of the level
	const uint32_t width;				// the width of the matrix
	const uint32_t depth;				// the depth of the matrix
	const uint32_t fingerprintLength;	// the fingerprint length
	const uint32_t row_addrs;			// the row addrs
	const uint32_t column_addrs;		// the column_addrs
	basket* value;						// the matrix

	int getMinIndex(uint32_t* a, int length);
public:
	Layer(uint32_t granularity, uint32_t width, uint32_t depth, uint32_t fingerprintLength, uint32_t row_addrs = 4, uint32_t column_addrs = 4);
	Layer(const Layer *layer);
	virtual ~Layer();
	uint32_t getGranularity() const;
	virtual void bucketCounting();

	virtual void insert(string src, string dst, weight_type weight) = 0;
	virtual weight_type edgeQuery(string src, string dst) = 0;
	virtual weight_type nodeQuery(string vertex, int type) = 0;		//src_type = 0 dst_type = 1
	
protected:
	bool insertMatrix(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst, weight_type weight);
	weight_type edgeQueryMatrix(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst);
	weight_type nodeQueryMatrix(uint32_t addr_v, uint16_t fp_v, int type);

	bool insertMatrixCacheline(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst, weight_type weight);
	weight_type edgeQueryMatrixCacheline(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst);
	weight_type nodeQueryMatrixCacheline(uint32_t addr_v, uint16_t fp_v, int type);
};

Layer::Layer(uint32_t granularity, uint32_t width, uint32_t depth, uint32_t fingerprintLength, uint32_t row_addrs, uint32_t column_addrs):
granularity(granularity), width(width), depth(depth), fingerprintLength(fingerprintLength), row_addrs(row_addrs), column_addrs(column_addrs) {
	cout << "Layer::Layer(granularity: " << granularity 
		<< ", width: " << width <<", depth: " << depth << ", fplen: " << fingerprintLength
		<< ", row_addrs: " << row_addrs << ", column_addrs: " << column_addrs << ")" << endl;

	uint32_t msize = width * depth;
	posix_memalign((void**)&value, 64, sizeof(basket) * msize); 	// 64-byte alignment of the requested space
	memset(this->value, 0, sizeof(basket) * msize);
}
Layer::Layer(const Layer *layer)
: granularity(2 * layer->getGranularity()), width(layer->width), depth(layer->depth), fingerprintLength(layer->fingerprintLength), row_addrs(layer->row_addrs), column_addrs(layer->column_addrs) {
	cout << "Layer::Layer(*layer)" << endl;
	
	uint32_t msize = width * depth;
	posix_memalign((void**)&value, 64, sizeof(basket) * msize);		// 64-byte alignment of the requested space
	for(uint32_t i = 0; i < msize; i++) {
		for(uint32_t j = 0; j < SLOTNUM; j++) {
			this->value[i].src[j] = layer->value[i].src[j];
			this->value[i].dst[j] = layer->value[i].dst[j];
			this->value[i].weight[j] = layer->value[i].weight[j];
		}
	}
}
Layer::~Layer() {
    cout << "Layer::~Layer()" << endl;
	delete[] this->value;
}

int Layer::getMinIndex(uint32_t* a, int length) {
	uint32_t min = a[0];
	int index = 0;
	for(int i = 1; i < length; i++) {
		if(a[i] < min) {
			min = a[i];
			index = i;
		}
	}
	return index;
}
uint32_t Layer::getGranularity() const{
	if(this == NULL) {
		cout << "NULL pointer!!" << endl;
		getchar();
		exit(-1);
	}
	return this->granularity;
}
void Layer::bucketCounting() {
    cout << "---------------------------------------" << endl;
    cout << "Layer bucketCounting(): print bucket..." << endl;
	int64_t room_count = 0;
	int64_t bucket_count = 0;
	for (int64_t i = 0; i < width * depth; i++) {
		if ((value[i].src[0] != 0) && (value[i].weight[0] != 0)) {
				bucket_count++;
		}
		for (int64_t j = 0; j < SLOTNUM; j++) {
			if ((value[i].src[j] != 0) && (value[i].weight[j] != 0)) {
				room_count++;
			}
		}
	}
	cout << "Layer room_count = " << room_count << ", total room = " << (width * depth * SLOTNUM) << ", space usage is " << 
			(double)room_count / (double)(width * depth * SLOTNUM) * 100 << "%" << endl;
	cout << "Layer bucket_count = " << bucket_count << ", total bucket = " << (width * depth) << ", space usage is " << 
			(double)bucket_count / (double)(width * depth) * 100 << "%" << endl;
	cout << "---------------------------------------" << endl;
	return;
}

bool Layer::insertMatrix(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst, weight_type weight) {
	uint32_t mask = (1 << fingerprintLength) - 1;
	uint32_t head = 16384; //pow(2, 14);

	// Alternative address -- row * column
	uint32_t* seed1 = new uint32_t[row_addrs];			// row address seeds
	uint32_t* seed2 = new uint32_t[column_addrs];		// column address seeds
	seed1[0] = fp_src;
	seed2[0] = fp_dst;
	for (int i = 1; i < row_addrs; i++)	
		seed1[i] =  (seed1[i - 1] * multiplier + increment) % modulus;
	for (int i = 1; i < column_addrs; i++)	
		seed2[i] =  (seed2[i - 1] * multiplier + increment) % modulus;

	for (int i = 0; i < row_addrs; i++) {
		uint32_t row_addr = (addr_src + seed1[i]) % depth;
		for (int j = 0; j < column_addrs; j++) {
			uint32_t column_addr = (addr_dst + seed2[j]) % width;
			uint32_t pos = row_addr * width + column_addr;
			for (int m = 0; m < SLOTNUM; m++) {
				if (((value[pos].src[m] >> 14) == i) && ((value[pos].dst[m] >> 14) == j) && ((value[pos].src[m] & mask) == fp_src) && ((value[pos].dst[m] & mask) == fp_dst)) {
					value[pos].weight[m] += weight;
					delete[] seed1;
					delete[] seed2;
					return true;
				}
				if (value[pos].src[m] == 0 && value[pos].weight[m] == 0) {
					value[pos].src[m] = fp_src + head * i;
					value[pos].dst[m] = fp_dst + head * j;
					value[pos].weight[m] = weight;
					delete[] seed1;
					delete[] seed2;
					return true;
				}
			}
		}
	}
	delete[] seed1;
	delete[] seed2;
	return false;
}
weight_type Layer::edgeQueryMatrix(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst) {
	uint32_t mask = (1 << fingerprintLength) - 1;
	// Alternative address -- row * column
	uint32_t* seed1 = new uint32_t[row_addrs];			// row address seeds
	uint32_t* seed2 = new uint32_t[column_addrs];		// column address seeds
	seed1[0] = fp_src;
	seed2[0] = fp_dst;
	for (int i = 1; i < row_addrs; i++)	
		seed1[i] =  (seed1[i - 1] * multiplier + increment) % modulus;
	for (int i = 1; i < column_addrs; i++)	
		seed2[i] =  (seed2[i - 1] * multiplier + increment) % modulus;

	for (int i = 0; i < row_addrs; i++) {
		uint32_t row_addr = (addr_src + seed1[i]) % depth;
		for (int j = 0; j < column_addrs; j++) {
			uint32_t column_addr = (addr_dst + seed2[j]) % width;
			uint32_t pos = row_addr * width + column_addr;
			if(pos >= width * depth || pos < 0) {
				cout << "matrix pos: " << pos << " out of range!" << endl;
				continue;
			}
			for (int m = 0; m < SLOTNUM; m++) {
				if (((value[pos].src[m] >> 14) == i) && ((value[pos].dst[m] >> 14) == j) && ((value[pos].src[m] & mask) == fp_src) && ((value[pos].dst[m] & mask) == fp_dst)) {
					delete[] seed1;
					delete[] seed2;
					return value[pos].weight[m];
				}
			}
		}
	}
	delete[] seed1;
	delete[] seed2;
	return 0;
}
weight_type Layer::nodeQueryMatrix(uint32_t addr_v, uint16_t fp_v, int type) {
	weight_type weight = 0;
	uint32_t mask = pow(2, fingerprintLength) - 1;
	int addrs = (type == 0) ? row_addrs : column_addrs;
	// Alternative address
	uint32_t* seeds = new uint32_t[addrs];			// address seeds
	seeds[0] = fp_v;
	for (int i = 1; i < addrs; i++)	
		seeds[i] =  (seeds[i - 1] * multiplier + increment) % modulus;
	
	if (type == 0) {
		for (int i = 0; i < row_addrs; i++)	{
			uint32_t row_addr = (addr_v + seeds[i]) % depth;
			for (int k = 0; k < width; k++)	{
				uint32_t pos = row_addr * width + k;
				for (int j = 0; j < SLOTNUM; ++j) {
					if (((value[pos].src[j] >> 14) == i) && ((value[pos].src[j] & mask) == fp_v)) {
						weight += value[pos].weight[j];
					}
				}
			}	
		}
	}
	else if (type == 1) {
		for (int i = 0; i < column_addrs; i++) {
			uint32_t col_addr = (addr_v + seeds[i]) % width;
			for (int k = 0; k < depth; k++) {
				uint32_t pos = k * width + col_addr;
				for (int j = 0; j < SLOTNUM; ++j) {
					if (((value[pos].dst[j] >> 14) == i) && ((value[pos].dst[j] & mask) == fp_v)) {
						weight += value[pos].weight[j];
					}
				}
			}
		}
	}
	delete[] seeds;
	return weight;
}

bool Layer::insertMatrixCacheline(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst, weight_type weight) {
	uint32_t mask = (1 << fingerprintLength) - 1;
	uint32_t head = 16384; //pow(2, 14);

	// Alternative address -- row * (column / 2) * 2 cache
	// the column_addrs must be even
	uint32_t* seed1 = new uint32_t[row_addrs];			// row address seeds
	uint32_t* seed2 = new uint32_t[column_addrs / 2];		// column address seeds
	seed1[0] = fp_src;
	seed2[0] = fp_dst;
	for (int i = 1; i < row_addrs; i++)	
		seed1[i] =  (seed1[i - 1] * multiplier + increment) % modulus;
	for (int i = 1; i < column_addrs / 2; i++)	
		seed2[i] =  (seed2[i - 1] * multiplier + increment) % modulus;

	for (int i = 0; i < row_addrs; i++) {
		uint32_t row_addr = (addr_src + seed1[i]) % depth;
		for (int j = 0; j < column_addrs / 2; j++) {
			uint32_t column_addr = (addr_dst + seed2[j]) % width;
			uint32_t column_addr_alt;
			for (int k = 0; k < 2; k++) {
				uint32_t pos;
				if (k == 0) {
					pos = row_addr * width + column_addr;
				}
				else {
					column_addr_alt = (column_addr ^ (fp_dst % column_addrs)) % width;
					pos = row_addr * width + column_addr_alt;
				}
				for (int m = 0; m < SLOTNUM; m++) {
					if (((value[pos].src[m] >> 14) == i) && ((value[pos].dst[m] >> 14) == (j * 2 + k)) && ((value[pos].src[m] & mask) == fp_src) && ((value[pos].dst[m] & mask) == fp_dst)) {
						value[pos].weight[m] += weight;
						delete[] seed1;
						delete[] seed2;
						return true;
					}
					if (value[pos].src[m] == 0 && value[pos].weight[m] == 0) {
						value[pos].src[m] = fp_src + head * i;
						value[pos].dst[m] = fp_dst + head * (j * 2 + k);
						value[pos].weight[m] = weight;
						delete[] seed1;
						delete[] seed2;
						return true;
					}
				}
			}
		}
	}
	delete[] seed1;
	delete[] seed2;
	return false;
}
weight_type Layer::edgeQueryMatrixCacheline(uint32_t addr_src, uint16_t fp_src, uint32_t addr_dst, uint16_t fp_dst) {
	uint32_t mask = (1 << fingerprintLength) - 1;

	// Alternative address -- row * (column / 2) * 2 cache
	// the column_addrs must be even
	uint32_t* seed1 = new uint32_t[row_addrs];			// row address seeds
	uint32_t* seed2 = new uint32_t[column_addrs / 2];		// column address seeds
	seed1[0] = fp_src;
	seed2[0] = fp_dst;
	for (int i = 1; i < row_addrs; i++)	
		seed1[i] =  (seed1[i - 1] * multiplier + increment) % modulus;
	for (int i = 1; i < column_addrs / 2; i++)	
		seed2[i] =  (seed2[i - 1] * multiplier + increment) % modulus;

	for (int i = 0; i < row_addrs; i++) {
		uint32_t row_addr = (addr_src + seed1[i]) % depth;
		for (int l = 0; l < column_addrs / 2; l++) {
			uint32_t column_addr = (addr_dst + seed2[l]) % width;
			for (int k = 0; k < 2; k++) {
				uint32_t pos;
				if (k == 0) {
					pos = row_addr * width + column_addr;
				}
				else {
					uint32_t column_addr_alt = (column_addr ^ (fp_dst % column_addrs)) % width;
					pos = row_addr * width + column_addr_alt;
				}
				if(pos >= width * depth || pos < 0) {
					cout << pos << " out of range!" << endl;
					continue;
				}
				for (int j = 0; j < SLOTNUM; j++) {
					if (((value[pos].src[j] >> 14) == i) && ((value[pos].dst[j] >> 14) == (l*2+k)) && ((value[pos].src[j] & mask) == fp_src) && ((value[pos].dst[j] & mask) == fp_dst)) {
						delete[] seed1;
						delete[] seed2;
						return value[pos].weight[j];
					}
				}
			}
		}
	}
	delete[] seed1;
	delete[] seed2;
	return 0;
}
weight_type Layer::nodeQueryMatrixCacheline(uint32_t addr_v, uint16_t fp_v, int type) {
	weight_type weight = 0;
	uint32_t mask = pow(2, fingerprintLength) - 1;
	int addrs = (type == 0) ? row_addrs : column_addrs / 2;
	// Alternative address
	uint32_t* seeds = new uint32_t[addrs];			// address seeds
	seeds[0] = fp_v;
	for (int i = 1; i < addrs; i++)	
		seeds[i] =  (seeds[i - 1] * multiplier + increment) % modulus;
	
	if (type == 0) {
		for (int i = 0; i < row_addrs; i++)	{
			uint32_t row_addr = (addr_v + seeds[i]) % depth;
			for (int k = 0; k < width; k++)	{
				uint32_t pos = row_addr * width + k;
				for (int j = 0; j < SLOTNUM; ++j) {
					if (((value[pos].src[j] >> 14) == i) && ((value[pos].src[j] & mask) == fp_v)) {
						weight += value[pos].weight[j];
					}
				}
			}	
		}
	}
	else if (type == 1) {
		// uint32_t py[4];
		// uint32_t addr = (hash_vertex >> fingerprintLength) % width;
		// py[0] = (addr + seed1[0]) % width;
		// py[1] = py[0] ^ (fp % 4) % width;
		// py[2] = (addr + seed1[1]) % width;
		// py[3] = py[2] ^ (fp % 4) % width;
		uint32_t* py = new uint32_t[column_addrs];
		for (int i = 0; i < column_addrs; i++) {
			if (i % 2 == 0) {
				py[i] = (addr_v + seeds[i / 2]) % width;
			}
			else {
				py[i] = py[i - 1] ^ (fp_v % column_addrs) % width;
			}
		}

		for (int k = 0; k < depth; k++) {
			for (int i = 0; i < column_addrs; i++) {
				uint32_t pos = k * width + py[i];
				for (int j = 0; j < SLOTNUM; ++j) {
					if (((value[pos].dst[j] >> 14) == i) && ((value[pos].dst[j] & mask) == fp_v)) {
						weight += value[pos].weight[j];
					}
				}
			}
		}
		delete[] py;
	}
	delete[] seeds;
	return weight;
}

#endif // _Layer_H