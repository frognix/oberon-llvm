#include "code_iterator.hpp"

/*!
 * \brief Точка возврата для парсера.
 *
 * Отвечает за сохранение точки в файле в которую парсер
 * может вернуться в случае неудачи.
 */
class BreakPoint {
  public:
    BreakPoint(CodeIterator& iter) : m_iter(iter) { m_place = m_iter.place(); }

    //! Сброс точки возврата
    void close() { state = CLOSED; }

    /*!
     * Деструктор производит возврат если это возможно.
     */
    ~BreakPoint() {
        if (m_iter.has_undroppable_error()) {
            state = CLOSED;
        } else if (state != CLOSED && !m_iter.can_move_to(m_place)) {
            state = CLOSED;
            m_iter.set_undroppable_error();
        }

        if (state == OPEN) {
            m_iter.move_to(m_place);
        }
    }

  private:
    //! Состояние точки возврата
    enum {
        CLOSED,  //!< Точка не действует
        OPEN,    //!< Точка действует
    } state = OPEN;
    CodeIterator& m_iter; //!< Поток к которому относится данная точка
    CodePlace m_place; //!< Индекс к которому необходимо вернуться
};
