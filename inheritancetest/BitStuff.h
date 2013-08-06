#include <stdint.h>


// Stores N bits huddled toward the MSB of M bits

/*
Number of bits we play with: N
Start of first byte to address: (M-N)/8
*/


#define bitGet(data, bit)  (((data)&(1<<(bit)))>>(bit))
#define bitSet(data, bit, val) (data = (data&(~(1<<(bit)))) | (((val)&0x1)<<(bit)))


template<int N, int M>
class BitFlags {
public:
	const uint8_t get( unsigned int idx ) {
		const int startByte = (M - N)/8;
		const int startBit = (M - N) - startByte*8;
		if( idx < N )
			return bitGet(data[startByte], startBit + idx);
		throw "Attempt to access memory out of range of this BitFlag!";
	}

	void set( unsigned int idx, const uint8_t val ) {
		const int startByte = (M - N)/8;
		const int startBit = (M - N) - startByte*8;
		if( idx < N )
			bitSet( data[startByte], startBit + idx, val );
		else
			throw "Attempt to modify memory out of range of this BitFlag!";
	}
private:
	// Store our data into here
	uint8_t data[M/8];
};


template<int N>
class uintX_t {
public:
	/*uintX_t( uint8_t val ) {
		this->val = val & ((1<<N) - 1);
	}*/

	uintX_t & operator=(uint8_t val) {
		this->val = val & ((1<<N) - 1);
	}

	operator uint8_t() const {
		return val;
	}
private:
	uint8_t val;
};


// An example of what this class is used like
/*
struct NodeType {
    union {
        // This is the type, but we only use 3 bits here
        uintX_t<3> type;

        // We've got 7 bits we can use for flags, this is a special type that packs 5 bits into an 8-bit space (prioritizing toward MSB)
        BitFlags<5,8> flags;
    };
};
*/
