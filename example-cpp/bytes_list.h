/*
 * Copyright (c) 2017, Sergey Stolyarov <sergei@regolit.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file pcsc_context.cpp
 * @author Sergey Stolyarov <sergei@regolit.com>
 */

#ifndef _H_86a8c08cde4be9654f3ac74da160267e
#define _H_86a8c08cde4be9654f3ac74da160267e

#include <string>

/**
 * @brief Класс для хранения и печати списка байтов
 */
class bytes_list
{
public:
    typedef enum {format_o, format_c, format_s} format_t;
    /**
     * @brief Создаёт пустой список.
     */
    bytes_list();

    /**
     * Создаёт список на основе байтового массива "src" длиной "size"
     */
    bytes_list(const unsigned char * src, size_t size);

    /**
     * Конструктор копирования
     */
    bytes_list(const bytes_list & src);
    ~bytes_list();

    /**
     * Возвращает список байтов в отформатированном виде,
     * @param f вид форматирования
     * @return байты в виде строки. Если format_c, то "0xab, 0x1e", 
     *         если format_o, то "AB 1E", 
     *         если format_s, то "\xab\x1e"
     */
    std::string str(format_t f = format_o) const;
    
    /**
     * Заменяет содержимое на байты из списка "src", старые байты уничтожаются.
     * @param src исходный список, из которого копируются байты
     */
    void replace(const bytes_list & src);

    /**
     * Возвращает общий размер списка.
     * @return размер списка в байтах, если список пустой, возвращается 0
     */
    size_t size() const;
    
    /**
     * Возвращает байт в позиции "index"
     * @throws std::out_of_range если "index" за границами списка байтов
     * @param  index индекс байта, нумерация с нуля
     * @return байт
     */
    unsigned char at(size_t index) const;

    /**
     * [byte_str description]
     * @param  byte [description]
     * @param  f    [description]
     * @return      [description]
     */
    static std::string byte_str(unsigned char byte, format_t f = format_o);

    /**
     * Возвращает строковое представление байта в позиции "index"
     * @throws std::out_of_range если "index" за границами списка байтов
     * @param  index индекс байта, нумерация с нуля
     * @param f см. str()
     * @return см. str()
     */
    std::string at_str(size_t index, format_t f = format_o) const;

    /**
     * Возвращает под-список с позиции "pos" длиной "length"
     * @param  pos    [description]
     * @param  length [description]
     * @return        [description]
     */
    bytes_list slice(size_t pos, size_t length) const;

private:
    struct Private;
    Private * p;
};

#endif