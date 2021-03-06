// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    binary maximum-heap with elements packed in [heapStart, heapEnd) - heap-sorted on > (*heapStart is the
    maximum element)

    note: internally, a T* heap is actually one before heapStart so that
    indices 1...heapSize can be used, so that 2*i is the left child of i, and
    2*i+1 is the right child.

    heapEnd - heapStart = number of elements
*/

#ifndef GRAEHL__SHARED__2HEAP_H
#define GRAEHL__SHARED__2HEAP_H

#include <cstdlib>
#include <iterator>
#include <algorithm>

#include <iostream>

namespace graehl {
template <typename T>
inline std::size_t heapSize(T s, T e) {
  return e - s;
}

template <typename T, typename V>
inline void heap_add(T heapStart, T heapEnd, V const& elt)
// caller is responsbile for ensuring that *heapEnd is allocated and
// safe to store the element in (and keeping track of increased size)
{
  std::size_t i = heapEnd - heapStart;
  std::size_t last = i;
  while ((i /= 2) && *(heapStart + i - 1) < elt) {
    *(heapStart + last - 1) = *(heapStart + i - 1);
    last = i;
  }
  *(heapStart + last - 1) = elt;
}

// internal routine: repair sub-heap condition given a violation at root element i (*(heap+i=1)==root)
template <typename T>
static inline void heapify(T heap, std::size_t heapSize, std::size_t i) {
  typename std::iterator_traits<T>::value_type temp = *(heap + i - 1);
  std::size_t parent = i, child = 2 * i;
  while (child < heapSize) {
    if (*(heap + child - 1) < *(heap + child)) ++child;
    if (!(temp < *(heap + child - 1))) break;
    *(heap + parent - 1) = *(heap + child - 1);
    parent = child;
    child *= 2;
  }
  if (child == heapSize && temp < *(heap + child - 1)) {
    *(heap + parent - 1) = *(heap + child - 1);
    parent = child;
  }
  *(heap + parent - 1) = temp;
}


template <typename T>
void heapPop(T heapStart, T heapEnd) {
  std::size_t size = heapSize(heapStart, heapEnd);
  *(heapStart) = *(heapStart + (--size));
  heapify(heapStart, size, 1);
}

template <typename T>
inline T& heapTop(T heapStart) {
  return *heapStart;
}


template <typename T>
void heapBuild(T heapStart, T heapEnd) {
  std::size_t size = heapEnd - heapStart;
  for (std::size_t i = size / 2; i; --i) heapify(heapStart, size, i);
}

// FIXME: test!
template <typename T>
bool heapVerify(T heapStart, T heapEnd) {
  std::size_t size = heapEnd - heapStart;
  while (--size > 1)
    if (*(heapStart + size / 2 - 1) < *(heapStart + size - 1)) return false;
  return true;
}

// *element may need to be moved up toward the root of the heap.  fix.
template <typename T>
inline void heapAdjustUp(T heapStart, T element) {
  std::size_t parent, current = element - heapStart + 1;
  typename std::iterator_traits<T>::value_type temp = *(heapStart + current - 1);
  while (current > 1) {
    parent = current / 2;
    if (!(*(heapStart + parent - 1) < temp)) break;
    *(heapStart + current - 1) = *(heapStart + parent - 1);
    current = parent;
  }
  *(heapStart + current - 1) = temp;
}


// *heapStart may need to be moved up toward the bottom of the heap.  fix.
template <typename T>
inline void heapAdjustDown(T heapStart, T heapEnd, T element) {
  heapify(heapStart, heapSize(heapStart, heapEnd), element - heapStart + 1);
}

// *heapStart may need to be moved up toward the bottom of the heap.  fix.
template <typename T>
inline void heapAdjustRootDown(T heapStart, T heapEnd) {
  heapAdjustDown(heapStart, heapEnd, heapStart[0]);
  heapify(heapStart, heapSize(heapStart, heapEnd), 1);
}

template <typename T>
void heapSort(T heapStart, T heapEnd) {
  heapBuild(heapStart, heapEnd);
  //  typename std::iterator_traits<T>::value_type temp;
  int heapSize = heapEnd - heapStart;
  for (int i = heapSize; i != 1; --i) {
    std::swap(*(heapStart), *(heapStart + i - 1));
    // temp = *(heap+1);*(heap+1) = *(heap+i);*(heap+i) = temp;
    heapify(heapStart, i - 1, 1);
  }
}

template <typename T>
void treeHeapAdd(T*& heapRoot, T* node) {
  T* oldRoot = heapRoot;
  if (!oldRoot) {
    heapRoot = node;
    node->left = node->right = NULL;
    node->nDescend = 0;
    return;
  }
  ++oldRoot->nDescend;
  int goLeft = !oldRoot->left || (oldRoot->right && oldRoot->right->nDescend > oldRoot->left->nDescend);
  if (*oldRoot < *node) {
    node->left = oldRoot->left;
    node->right = oldRoot->right;
    node->nDescend = oldRoot->nDescend;
    heapRoot = node;
    if (goLeft)
      treeHeapAdd(node->left, oldRoot);
    else
      treeHeapAdd(node->right, oldRoot);
  } else {
    if (goLeft)
      treeHeapAdd(oldRoot->left, node);
    else
      treeHeapAdd(oldRoot->right, node);
  }
}


template <typename T>
T* newTreeHeapAdd(T* heapRoot, T* node) {
  if (!heapRoot) {
    node->left = node->right = NULL;
    node->nDescend = 0;
    return node;
  }
  T* newRoot = new T(*heapRoot);
  ++newRoot->nDescend;
  bool goLeft = !newRoot->left || (newRoot->right && newRoot->right->nDescend > newRoot->left->nDescend);
  if (*newRoot < *node) {
    node->left = newRoot->left;
    node->right = newRoot->right;
    node->nDescend = newRoot->nDescend;
    if (goLeft)
      node->left = newTreeHeapAdd(node->left, newRoot);
    else
      node->right = newTreeHeapAdd(node->right, newRoot);
    return node;
  } else {
    if (goLeft)
      newRoot->left = newTreeHeapAdd(newRoot->left, node);
    else
      newRoot->right = newTreeHeapAdd(newRoot->right, node);
    return newRoot;
  }
}

// (vector) container versions (require that begin and end be C::value_type *)
template <typename C>
inline C& heapTop(const C& heap) {
  return heap.front();
}

template <typename C>
inline void heap_pop(C& heap) {
  heapPop(heap.begin(), heap.end());
  heap.pop_back();
}

template <typename C, typename V>
inline void heap_add(C& heap, const V& elt) {
  heap.push_back(elt);
  heap_add(heap.begin(), heap.end(), elt);
}

template <typename C>
inline void heapBuild(C& heap) {
  heapBuild(heap.begin(), heap.end());
}

template <typename C>
inline void heapAdjustUp(C& heap, typename C::iterator element) {
  heapAdjustUp(heap.begin(), element);
}

template <typename C>
inline void heapSort(C& heap) {
  heapSort(heap.begin(), heap.end());
}
}  // graehl

#endif
