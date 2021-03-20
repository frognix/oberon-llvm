#include "code_stream.hpp"
#include "parse_error.hpp"

/*!
 * \brief Точка возврата для парсера.
 *
 * Отвечает за сохранение точки в файле в которую парсер
 * может вернуться в случае неудачи.
 */
class BreakPoint {
  public:
    BreakPoint(CodeStream& stream) : m_stream(stream) { m_index = m_stream.index(); }

    //! Сброс точки возврата
    void close() { state = CLOSED; }

    /*!
     * \brief Передача ошибки из парсера.
     *
     * В случае ошибки парсер должен передать её в точку возрата
     * которая определит возможен ли возврат после этой ошибки или нет.
     */
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

    /*!
     * Деструктор производит возврат если это возможно.
     */
    ~BreakPoint() {
        if (state == OPEN) {
            m_stream.move_to(m_index);
        } else if (state == INVALID) {
            fmt::print("Bad usage of breakpoint!\n");
            std::terminate();
        }
    }

  private:
    //! Состояние точки возврата
    enum {
        INVALID, //!< Невалидное состояние
        CLOSED,  //!< Точка не действует
        OPEN,    //!< Точка действует
    } state = INVALID;
    CodeStream& m_stream; //!< Поток к которому относится данная точка
    size_t m_index; //!< Индекс к которому необходимо вернуться
};
