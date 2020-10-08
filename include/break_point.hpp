#include "code_stream.hpp"

class BreakPoint {
public:
    BreakPoint(CodeStream& stream) : m_stream(stream) {
        m_point = m_stream.m_file.tellg();
        m_place = m_stream.m_place;
    }
    ~BreakPoint() {
        if (!closed) {
            m_stream.m_file.clear();
            m_stream.m_file.seekg(m_point);
            m_stream.m_place = m_place;
        }
    }
    void close() { closed = true; }
private:
    bool closed = false;
    CodeStream& m_stream;
    std::fstream::pos_type m_point;
    CodePlace m_place;
};
