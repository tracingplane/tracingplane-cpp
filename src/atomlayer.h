#ifndef _ATOM_LAYER_H_
#define _ATOM_LAYER_H_

#include <iostream>
#include <cstdint>
#include <cstddef>
#include <vector>

std::vector<uint8_t> encodeVarint(uint64_t x) {
	std::vector<uint8_t> s;
	while (x >= 128) {
		uint8_t c = x | 0x80;
		s.push_back(c);
		x >>= 7;
	}
	s.push_back(x);
	return s;
}

uint64_t decodeVarint(std::vector<uint8_t> s, unsigned &n) {
	uint64_t x = 0;
	for (unsigned shift = 0; shift < 64; shift += 7) {
		if (n > s.size()) {
			return 0;
		}
		uint8_t b = s[n];
		n++;
		x |= ((uint64_t) (b & 0x7F)) << shift;
		if ((b & 0x80) == 0) {
			return x;
		}
	}
}


struct Atom {

	std::vector<uint8_t> bytes;

	Atom() : bytes(0) {}

	Atom(int x) : bytes(4) {
		for (unsigned i = 0; i < 4; i++) {
			bytes[3-i] = (x >> (i * 8));
		}
	}

	Atom(std::vector<uint8_t> bytes) : bytes(bytes) {}

	const int compare(const Atom& other) {
		for (unsigned i = 0; i < bytes.size() && i < other.bytes.size(); i++) {
			if (bytes[i] < other.bytes[i]) {
				return -1;
			} else if (bytes[i] > other.bytes[i]) {
				return 1;
			}
		}
		if (bytes.size() < other.bytes.size()) {
			return -1;
		} else if (bytes.size() > other.bytes.size()) {
			return 1;
		} else {
			return 0;
		}
	}

	friend std::ostream& operator<< ( std::ostream& os, const Atom& atom ) {
		os << "[";
		bool first = true;
		for (unsigned i = 0; i < atom.bytes.size(); i++) {
			if (!first) os << " ";
			first = false;

			os << unsigned(atom.bytes[i]);
		}
		if (first) os << "-";
		os << "]";
	}


};


struct Baggage {

	std::vector<Atom> atoms;

	Baggage() : atoms(0) {}

	Baggage branch() {
		Baggage duplicate;
		duplicate.atoms = atoms;
		return duplicate;
	}

	static Baggage merge(Baggage& a, Baggage& b) {
		Baggage merged;
		unsigned i = 0, j = 0;
		while (i < a.atoms.size() && j < b.atoms.size()) {
			switch(a.atoms[i].compare(b.atoms[j])) {
				case -1: merged.atoms.push_back(a.atoms[i]); i++; break;
				case 0: merged.atoms.push_back(a.atoms[i]); i++; j++; break;
				case 1: merged.atoms.push_back(b.atoms[j]); j++; break;
			}
		}
		for (; i < a.atoms.size(); i++) {
			merged.atoms.push_back(a.atoms[i]);
		}
		for (; j < b.atoms.size(); j++) {
			merged.atoms.push_back(b.atoms[j]);
		}
		return merged;
	}

	std::vector<uint8_t> serialize() {
		std::vector<uint8_t> bytes;
		for (unsigned i = 0; i < atoms.size(); i++) {
			std::vector<uint8_t> size_prefix = encodeVarint(atoms[i].bytes.size());
			bytes.insert(bytes.end(), size_prefix.begin(), size_prefix.end());
			bytes.insert(bytes.end(), atoms[i].bytes.begin(), atoms[i].bytes.end());
		}
		return bytes;
	}

	static Baggage deserialize(std::vector<uint8_t> bytes) {
		Baggage baggage;
		unsigned n = 0;
		while (n < bytes.size()) {
			uint64_t next_atom_size = decodeVarint(bytes, n);
			Atom a(std::vector<uint8_t>(bytes.begin() + n, bytes.begin() + n + next_atom_size));
			baggage.atoms.push_back(a);
			n += next_atom_size;
		}
		return baggage;
	}


	friend std::ostream& operator<< ( std::ostream& os, const Baggage& baggage ) {
		os << "[";
		bool first = true;
		for (unsigned i = 0; i < baggage.atoms.size(); i++) {
			if (!first) os << " ";
			first = false;

			os << baggage.atoms[i];
		}
		os << "]";
	}

};


#endif