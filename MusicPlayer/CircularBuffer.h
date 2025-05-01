#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <vector>
#include <cstddef>
#include <algorithm>

class CircularBuffer
{
public:
    explicit CircularBuffer(size_t capacity)
        : buffer(capacity, 0.0f), capacity(capacity), head(0), count(0)
    {}

    // Push a block of samples into the circular buffer.
    // If new samples exceed capacity, they will overwrite the oldest samples.
    void pushSamples(const float* samples, size_t numSamples)
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            buffer[head] = samples[i];
            head = (head + 1) % capacity;
            if (count < capacity)
                ++count;
        }
    }

    // Get a pointer to the internal buffer in logical order.
    // Note: This copies the logical ordering into a provided vector.
    void getBuffer(std::vector<float>& outBuffer) const
    {
        outBuffer.resize(count);
        size_t tail = (head + capacity - count) % capacity;
        for (size_t i = 0; i < count; ++i)
            outBuffer[i] = buffer[(tail + i) % capacity];
    }

    // Clear the buffer.
    void clear()
    {
        head = 0;
        count = 0;
    }

    // Return the number of samples currently stored.
    size_t size() const { return count; }

    // Return the fixed capacity.
    size_t getCapacity() const { return capacity; }

private:
    std::vector<float> buffer;
    size_t capacity;
    size_t head;   // The index where new samples will be written.
    size_t count;  // Number of valid samples currently stored.
};

#endif // CIRCULARBUFFER_H
