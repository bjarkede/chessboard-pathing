#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

template <typename T>
class DynamicDeque {
private:
  T * buffer;
  T * old_buffer;
  int head, tail;
  int maxNum;
  int numEntries;

  void reallocate(int num);
  void error(int e, int n);
  DynamicDeque(DynamicDeque const&) {};
  void operator = (DynamicDeque const&) {};
public:
  DynamicDeque();
  ~DynamicDeque();
  void reserve(int num);
  int getnum() {return numEntries;};
  int getmaxnum() {return maxNum;};
  
  void push_back(T const & obj);
  void push_front(T const & obj);
  T pop_front();
  T pop_back();

  void flush();

  T & operator[] (int i);
  enum DefineSize {
		   AllocateSpace = 1024	   
  };
};

template <typename T>
DynamicDeque<T>::DynamicDeque() {
  // Constructor
  buffer = old_buffer = 0;
  maxNum = numEntries = head = tail = 0;
}

template <typename T>
DynamicDeque<T>::~DynamicDeque() {
  // Destructor
  reserve(0); // De-allocate the buffer
}

template <typename T>
void DynamicDeque<T>::reserve(int num) {
  if(num <= maxNum) {              // If num <= the current maxNum do nothing.
    if(num <= 0) {                 // Discard all data and de-allocate the buffer
      if (num < 0) error(1, num);
      if (buffer) delete[] buffer; // De-allocate buffer
      buffer = 0;                  // Reset
      maxNum = numEntries = head = tail = 0;
      return;
    }

    return;
  }

  // If num is greater than maxNUm increase the buffer
  reallocate(num);
  if(old_buffer) {delete[] old_buffer; old_buffer = 0;} // Delete the old_buffer after re-allocation.
}

template <typename T>
void DynamicDeque<T>::reallocate(int num) {
  if(old_buffer) delete[] old_buffer;

  T * new_buffer = 0;
  new_buffer = new T[num]; // Allocate the new buffer
  if(new_buffer == 0) {error(3,num); return;}
  if(buffer) {
    // Handle the situation where a smaller buffer
    // was previously allocated, and copy the deque from the old to the new.
    if(tail < head) {
      memcpy(new_buffer, buffer + tail, numEntries*sizeof(T));
    }
    else if (numEntries) {
      // Wrapping around or full.
      memcpy(new_buffer, buffer + tail, (maxNum - tail)*sizeof(T));
      if (head) {
	memcpy(new_buffer + (maxNum - tail), buffer, head*sizeof(T));
      }
    }

    // Reset head and tail
    tail = 0; head = numEntries;
  }
  old_buffer = buffer;
  buffer     = new_buffer;
  maxNum     = num;
}

template <typename T>
void DynamicDeque<T>::push_back(T const & obj) {
  // If the buffer is to small or if there is no buffer. Allocate more memory.
  if (numEntries >= maxNum) {
    int newSize = maxNum * 2 + (AllocateSpace+sizeof(T)-1/sizeof(T));
    reallocate(newSize);
  }

  // Insert new object at tail
  buffer[head] = obj;
  if (old_buffer) {delete[] old_buffer; old_buffer = 0;} // Delete the old buffer after copy, in case obj was in old buffer.
  if (++head >= maxNum) head = 0;
  numEntries++;                                          // Increment the amount of entries  
}

template <typename T>
void DynamicDeque<T>::push_front(T const & obj) {
  // If the buffer is to small or if there is no buffer. Allocate more memory.
  if (numEntries >= maxNum) {
    int newSize = maxNum * 2 + (AllocateSpace+sizeof(T)-1/sizeof(T));
    reallocate(newSize);
  }

  // Move the buffer 1 address to the right
  memmove(&buffer[1], &buffer[0], numEntries*sizeof(T));
  
  buffer[tail] = obj;
  if (old_buffer) {delete[] old_buffer; old_buffer = 0;}
  if(++head >= maxNum) head = 0;
  
  numEntries++;  
}

template <typename T>
T DynamicDeque<T>::pop_front() {
  if (numEntries <= 0) { // Buffer is empty
    error(2, 0);
    T temp;
    memset(&temp, 0, sizeof(temp));
    return temp;        // Return empty object
  }

  // Pointer to the object at head
  T * p = buffer + (head-1);
  if (--head <= 0) head = 0;
  numEntries--;
  return *p;
}

template <typename T>
T DynamicDeque<T>::pop_back() {
  if (numEntries <= 0) { // Buffer is empty
    error(2, 0);
    T temp;
    memset(&temp, 0, sizeof(temp));
    return temp;        // Return empty object
  }

  T * p = buffer + tail; // Tail is one in front
  if(++tail >= maxNum) tail = 0;
  numEntries--;
  return *p;
}

template <typename T>
void DynamicDeque<T>::flush() {
  if(!buffer) {
    return;
  }
  
  if (buffer) delete[] buffer; // De-allocate buffer
  buffer = 0;                  // Reset
  maxNum = numEntries = head = tail = 0; // Reset the indexes

  if(old_buffer) {delete[] old_buffer; old_buffer = 0;} // Delete the old buffer

  return;
}

template <typename T>
T & DynamicDeque<T>::operator[] (int i) {
  // Access the object at position i from tail
  if ((unsigned int)i >= (unsigned int)numEntries) {
    error(1, i); i = 0; // Index i does not exist.
  }

  // Calculate the position
  i += tail;
  if (i >= maxNum) i -= maxNum; // Wrap around
  return buffer[i];
}

template <typename T>
void DynamicDeque<T>::error(int e, int n) {
  // Define the error texts
  static const char * errors[] = {
				  "Unknown error",
				  "Index out of range",
				  "Deque is empty",
				  "Memory allocation failed"
  };

  // Handle the error.
  const unsigned int numErrors = sizeof(errors) / sizeof(*errors);
  if ((unsigned int)e >= numErrors) e = 0; // If the error is out of index, set to unknown error
  fprintf(stderr, "\nDynamicDeque error: %s (%i)\n", errors[e], n);

  // Terminate execution
  exit(1);
}
