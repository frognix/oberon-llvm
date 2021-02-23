#include "code_stream.hpp"
#include "parse_error.hpp"

class BreakPoint {
  public:
    BreakPoint(CodeStream& stream) : m_stream(stream) { m_index = m_stream.index(); }

    void close() { state = CLOSED; }

    void error(ParseError& err) {
        if (err.is_undroppable()) {
            state = CLOSED;
        } else if (!m_stream.can_move_to(m_index)) {
            state = CLOSED;
            err.set_undroppable();
        } else {
            state = OPEN;
        }
    }

    ~BreakPoint() {
        if (state == OPEN) {
            m_stream.move_to(m_index);
        } else if (state == INVALID) {
            fmt::print("Bad usage of breakpoint!\n");
            std::terminate();
        }
    }

  private:
    enum {
        INVALID,
        CLOSED,
        OPEN,
    } state = INVALID;
    CodeStream& m_stream;
    size_t m_index;
};
