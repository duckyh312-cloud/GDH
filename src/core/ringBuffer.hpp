#pragma once

#include <vector>
#include <functional>
#include <algorithm>

template <typename T>
class RingBuffer {
private:
    std::vector<T> m_buffer;
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_maxSize = 0;
    bool m_isFull = false;

    std::function<void(T&)> m_onOverwrite = nullptr;
    std::function<void(T&)> m_onClear = nullptr;

public:
    RingBuffer() = default;
    
    RingBuffer(size_t size, 
               std::function<void(T&)> onOverwrite = nullptr, 
               std::function<void(T&)> onClear = nullptr) {
        init(size, onOverwrite, onClear);
    }

    void init(size_t size, 
              std::function<void(T&)> onOverwrite = nullptr, 
              std::function<void(T&)> onClear = nullptr) {
        clear();
        m_maxSize = size;
        m_buffer.resize(size);
        m_onOverwrite = onOverwrite;
        m_onClear = onClear;
        m_head = 0;
        m_tail = 0;
        m_isFull = false;
    }

    void push(const T& item) {
        if (m_maxSize == 0 || m_buffer.empty()) return;

        if (m_isFull) {
            if (m_onOverwrite) {
                m_onOverwrite(m_buffer[m_head]);
            }
            m_tail = (m_tail + 1) % m_maxSize;
        }

        m_buffer[m_head] = item;
        m_head = (m_head + 1) % m_maxSize;

        if (m_head == m_tail) {
            m_isFull = true;
        }
    }

    bool pop_back(T& outItem) {
        if (empty()) return false;

        if (m_head == 0) {
            m_head = m_maxSize - 1;
        } else {
            m_head--;
        }

        outItem = m_buffer[m_head];
        m_buffer[m_head] = T();
        m_isFull = false;
        return true;
    }

    bool back(T& outItem) const {
        if (empty()) return false;
        size_t lastIdx = (m_head == 0) ? (m_maxSize - 1) : (m_head - 1);
        outItem = m_buffer[lastIdx];
        return true;
    }

    bool empty() const {
        return (m_head == m_tail && !m_isFull);
    }

    bool full() const {
        return m_isFull;
    }

    size_t size() const {
        if (m_isFull) return m_maxSize;
        if (m_head >= m_tail) return m_head - m_tail;
        return m_maxSize + m_head - m_tail;
    }

    void clear() {
        if (m_onClear) {
            for (auto& item : m_buffer) {
                m_onClear(item);
            }
        }
        std::fill(m_buffer.begin(), m_buffer.end(), T());
        m_head = 0;
        m_tail = 0;
        m_isFull = false;
    }

    void for_each(const std::function<void(const T&)>& action) const {
        size_t count = size();
        size_t current = m_tail;
        for (size_t i = 0; i < count; ++i) {
            action(m_buffer[current]);
            current = (current + 1) % m_maxSize;
        }
    }
};