/*******************************************************************************
Description:  Implements a bit vector class.
Author:       Ghassan Shobaki
Created:      Jun. 2002
Last Update:  Mar. 2011
*******************************************************************************/

#ifndef OPTSCHED_GENERIC_BIT_VECTOR_H
#define OPTSCHED_GENERIC_BIT_VECTOR_H

#include "opt-sched/Scheduler/logger.h"
#include "opt-sched/Scheduler/mem_mngr.h"
#include <cstring>
#include <memory>

namespace llvm {
namespace opt_sched {

class BitVector {
public:
  // The actual integral type that is used to store the bits.
  typedef unsigned int Unit;

  // Constructs a bit vector of a given length.
  BitVector(int length = 0);
  // Deallocates the vector.
  virtual ~BitVector();

  // Reconstructs the vector to hold a vector of the new length. All old data
  // is discarded.
  void Construct(int length);

  // Sets all bits to 0.
  virtual void Reset();
  // Sets the bit at the given index to the given value.
  void SetBit(int index, bool value = true);
  // Returns the value of the bit at a given index.
  bool GetBit(int index) const;
  // Returns the number of one bits in the bit vector.
  int GetOneCount() const;
  // Returns the number of bits in the vector.
  int GetSize() const;
  // Create a bit vector that is the "bitwise and" of this bit vector and
  // another bit vector.
  std::unique_ptr<BitVector> And(BitVector *otherBitVector) const;
  // Returns true if this BitVector's one bits are a subset of "otherBitVector".
  bool IsSubVector(BitVector *otherBitVector) const;

  // Assigns the values from src to the vector. Both vectors must be of the
  // same size.
  BitVector &operator=(const BitVector &src);
  // Compares two bit vectors.
  bool operator==(const BitVector &othr) const;

protected:
  // The buffer in which the bits are stored.
  Unit *vctr_;
  // The number of bits.
  int bitCount_;
  // The number of units of the actual integer data type used.
  int unitCount_;
  // The number of ones currently in the vector.
  int oneCount_;

  // Gets a Unit-sized bitmask for a given bit, inverted if val = false.
  static Unit GetMask_(int bitNum, bool val);
  // The number of bits per storage unit.
  static const int BITS_IN_UNIT = sizeof(Unit) * 8;
};

inline BitVector::BitVector(int length) {
  bitCount_ = 0;
  unitCount_ = 0;
  oneCount_ = 0;
  vctr_ = NULL;
  Construct(length);
}

inline void BitVector::Construct(int length) {
  bitCount_ = length;
  unitCount_ = (bitCount_ + BITS_IN_UNIT - 1) / BITS_IN_UNIT;

  if (unitCount_ == 0)
    return;

  if (vctr_)
    delete[] vctr_;
  vctr_ = new Unit[unitCount_];

  for (int i = 0; i < unitCount_; i++) {
    vctr_[i] = 0;
  }

  oneCount_ = 0;
}

inline BitVector::~BitVector() {
  if (vctr_ != NULL)
    delete[] vctr_;
}

inline void BitVector::Reset() {
  if (oneCount_ == 0)
    return;

  for (int i = 0; i < unitCount_; i++) {
    vctr_[i] = 0;
  }

  oneCount_ = 0;
}

inline void BitVector::SetBit(int index, bool bitVal) {
  assert(index < bitCount_);
  int unitNum = index / BITS_IN_UNIT;
  int bitNum = index - unitNum * BITS_IN_UNIT;
  Unit mask = GetMask_(bitNum, bitVal);

  if (bitVal) {
    if (GetBit(index) == false)
      oneCount_++;
    vctr_[unitNum] |= mask;
  } else {
    if (GetBit(index) == true)
      oneCount_--;
    vctr_[unitNum] &= mask;
  }
}

inline bool BitVector::GetBit(int index) const {
  assert(index < bitCount_);
  int unitNum = index / BITS_IN_UNIT;
  int bitNum = index - unitNum * BITS_IN_UNIT;
  return (vctr_[unitNum] & GetMask_(bitNum, true)) != 0;
}

inline bool BitVector::IsSubVector(BitVector *other) const {
  assert(other != NULL);
  // The other vector must be at least as large as this vector.
  if (unitCount_ > other->unitCount_)
    return false;

  for (int i = 0; i < unitCount_; i++) {
    if ((vctr_[i] & other->vctr_[i]) != vctr_[i])
      return false;
  }
  return true;
}

inline std::unique_ptr<BitVector>
BitVector::And(BitVector *otherBitVector) const {
  assert(otherBitVector != NULL);
  // Set length to the larger of the two vectors
  int bitCount = bitCount_ > otherBitVector->bitCount_
                     ? bitCount_
                     : otherBitVector->bitCount_;
  std::unique_ptr<BitVector> andedVector(new BitVector(bitCount));

  for (int i = 0; i < andedVector->unitCount_; i++) {
    andedVector->vctr_[i] = vctr_[i] & otherBitVector->vctr_[i];

    // TODO(austin) This may not be portable enough.
    // This is a built in gcc function for counting the number of 1 bits
    // in a number. When using x86 it should be inplemented as a single
    // instruction ie "popcnt %rdi, %rax"
    andedVector->oneCount_ += __builtin_popcountll(andedVector->vctr_[i]);
  }

  return andedVector;
}

inline int BitVector::GetSize() const { return bitCount_; }

inline int BitVector::GetOneCount() const { return oneCount_; }

inline BitVector &BitVector::operator=(const BitVector &src) {
  assert(bitCount_ == src.bitCount_);
  int byteCount = unitCount_ * sizeof(Unit);
  memcpy(vctr_, src.vctr_, byteCount);
  oneCount_ = src.oneCount_;
  return *this;
}

inline bool BitVector::operator==(const BitVector &other) const {
  assert(bitCount_ == other.bitCount_);
  if (oneCount_ != other.oneCount_)
    return false;
  int byteCount = unitCount_ * sizeof(Unit);
  return (memcmp(vctr_, other.vctr_, byteCount) == 0);
}

inline BitVector::Unit BitVector::GetMask_(int bitNum, bool bitVal) {
  assert(bitNum < BITS_IN_UNIT);
  Unit mask = ((Unit)1) << bitNum;

  if (!bitVal) {
    // The mask for setting a bit to 0 is the inverse of the mask for setting a
    // bit to 1. E.g. ORing with the mask 0x0008 sets the fourth bit to 1 while
    // ANDing with the mask 0xfff7 sets the fourth bit to 0.
    mask = ~mask;
  }

  return mask;
}

// Used to track weighted spill cost where a live register can have a weight
// that increases the cost of the register being live proportional to its
// weight.
class WeightedBitVector : public BitVector {
public:
  WeightedBitVector(int lenght = 0);
  ~WeightedBitVector();
  void SetBit(int index, bool bitVal, int weight);
  int GetWghtedCount() const;
  virtual void Reset() override;

private:
  // The weighted sum of 1 in the vector times their weight
  int wghtedCount_;
};

inline WeightedBitVector::WeightedBitVector(int length) : BitVector(length) {
  wghtedCount_ = 0;
}

inline WeightedBitVector::~WeightedBitVector() {}

inline void WeightedBitVector::SetBit(int index, bool bitVal, int weight) {
  assert(index < bitCount_);
  int unitNum = index / BITS_IN_UNIT;
  int bitNum = index - unitNum * BITS_IN_UNIT;
  Unit mask = GetMask_(bitNum, bitVal);

  if (bitVal) {
    if (GetBit(index) == false) {
      oneCount_++;
      wghtedCount_ += weight;
    }
    vctr_[unitNum] |= mask;
  } else {
    if (GetBit(index) == true) {
      oneCount_--;
      wghtedCount_ -= weight;
    }
    vctr_[unitNum] &= mask;
  }
}

inline int WeightedBitVector::GetWghtedCount() const { return wghtedCount_; }

inline void WeightedBitVector::Reset() {
  BitVector::Reset();
  wghtedCount_ = 0;
}

/*
void BitVector::Print(FILE* file) {
  fprintf(file, "Bit Vector (size=%d, unitCount_=%d, oneCount_=%d): ",
          bitCount_, unitCount_, oneCount_);

  for (int i = 0; i < bitCount_; i++) {
    if (GetBit(i)) {
      fprintf(file, "1");
    } else {
      fprintf(file, "0");
    }
  }

  fprintf(file, "\n");
}
*/

} // namespace opt_sched
} // namespace llvm

#endif
