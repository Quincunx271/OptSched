#ifndef OPTSCHED_ARRAY_REF_2D_H
#define OPTSCHED_ARRAY_REF_2D_H

#include "llvm/ADT/ArrayRef.h"
#include <cassert>
#include <cstddef>

namespace llvm {
namespace opt_sched {
template <typename T> class ArrayRef2D {
public:
  explicit ArrayRef2D(llvm::ArrayRef<T> Ref, size_t Rows, size_t Columns)
      : Ref(Ref), Rows(Rows), Columns(Columns) {
    assert(Rows * Columns == Ref.size());
  }

  size_t rows() const { return Rows; }
  size_t columns() const { return Columns; }

  const T &operator[](size_t(&&RowCol)[2]) const {
    return Ref[computeIndex(RowCol[0], RowCol[1], Rows, Columns)];
  }

  llvm::ArrayRef<T> underlyingData() const { return Ref; }

private:
  llvm::ArrayRef<T> Ref;
  size_t Rows;
  size_t Columns;

  static size_t computeIndex(size_t row, size_t col, size_t Rows,
                             size_t Columns) {
    assert(row < Rows && "Invalid row");
    assert(col < Columns && "Invalid column");
    size_t index = row * Columns + col;
    assert(index < Rows * Columns); // Should be redundant with prior asserts.
    return index;
  }
};

template <typename T> class MutableArrayRef2D : public ArrayRef2D<T> {
public:
  explicit MutableArrayRef2D(llvm::MutableArrayRef<T> Ref, size_t Rows,
                             size_t Columns)
      : ArrayRef2D<T>(Ref, Rows, Columns) {}

  T &operator[](size_t(&&RowCol)[2]) const {
    ArrayRef2D<T> cref = *this;
    return const_cast<T &>(cref[{RowCol[0], RowCol[1]}]);
  }

  llvm::MutableArrayRef<T> underlyingData() const {
    return static_cast<const llvm::MutableArrayRef<T> &>(
        ArrayRef2D<T>::underlyingData());
  }
};
} // namespace opt_sched
} // namespace llvm

#endif
