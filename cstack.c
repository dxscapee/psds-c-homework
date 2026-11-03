#include "cstack.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// добавил из technical-requirement.md
// ===========================================
struct node
{
    const struct node* prev;
    unsigned int size;
    char data[];
};
typedef struct node* stack_t;

struct stack_entry
{
    int reserved;
    stack_t stack;
};
typedef struct stack_entry stack_entry_t;

struct stack_entries_table
{
    unsigned int size;
    stack_entry_t* entries;
};

/*
 *Такой момент: я решил сделать таблицу статичной. Исходя из лекции 3 ноября
сделал вывод, что таблицу стоило бы сделать статической, чтобы ограничить к ней
доступ "из вне" (external linkage). Если я всё таки не прав по этому поводу -
прошу сообщить :)
*/
static struct stack_entries_table g_table = {0u, NULL};

//==============================================================================

hstack_t stack_new(void)
{
    // идём по всем вхождениям, ищем среди них не зарезервированные
    size_t index;
    for (index = 0u; index < g_table.size; index++)
    {
        if (!g_table.entries[index].reserved)
        {
            g_table.entries[index].reserved = 1;
            g_table.entries[index].stack = NULL;
            break;
        }
    }
    hstack_t handler = (hstack_t)(index);

    // если среди вхождений не нашли свободное - перевыделяем память и добавляем
    // новые вхождения
    if (index >= g_table.size)
    {
        unsigned int old_size = g_table.size;
        unsigned int new_size = old_size == 0 ? 10u : old_size * 2u;

        // realloc перевыделит память если закончили слоты или выделит, если их
        // еще не было
        stack_entry_t* new_entries =
            realloc(g_table.entries, (size_t)new_size * sizeof(*new_entries));
        if (new_entries == NULL)
        {
            return (hstack_t)-1;
        }
        g_table.entries = new_entries;
        g_table.size = new_size;

        for (size_t i = old_size; i < new_size; ++i)
        {
            new_entries[i].reserved = 0;
            new_entries[i].stack = NULL;
        }

        new_entries[index].reserved = 1;
        new_entries[index].stack = NULL;
    }
    return handler;
}

void stack_free(const hstack_t hstack)
{
    // если наш хендлер некорректный
    if (!stack_valid_handler(hstack))
        return;

    stack_t curr = g_table.entries[hstack].stack;

    while (curr != NULL)
    {
        stack_t prev = (stack_t)curr->prev;
        free(curr);
        curr = prev;
    }
    g_table.entries[hstack].reserved = 0;
    g_table.entries[hstack].stack = NULL;
}

void stack_free_all(void)
{
    if (g_table.entries == NULL || g_table.size == 0)
        return;

    // проверка: если все вхождения пустые - не нужно ничего делать
    // если убрать проверку - сработает блок из конца функции
    unsigned int FLAG = 1;
    for (size_t i = 0u; i < g_table.size; ++i)
    {
        if (g_table.entries[i].reserved == 1)
        {
            FLAG = 0;
        }
    }
    if (FLAG == 1)
        return;

    for (size_t i = 0u; i < g_table.size; ++i)
    {
        if (g_table.entries[i].reserved != 0)
            stack_free((hstack_t)i);
    }

    free(g_table.entries);
    g_table.entries = NULL;
    g_table.size = 0;
}

int stack_valid_handler(const hstack_t hstack)
{
    if (hstack < 0)
        return 0;

    if (g_table.entries == NULL)
        return 0;

    if ((unsigned int)hstack >= g_table.size)
        return 0;

    if (!g_table.entries[hstack].reserved)
        return 0;

    return 1;
}

unsigned int stack_size(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
        return 0;

    unsigned int count = 0u;
    const struct node* curr = g_table.entries[hstack].stack;

    // проходим по всему стеку и пока узел не NULL просто прибавляем count
    while (curr != NULL)
    {
        ++count;
        curr = curr->prev;
    }

    return count;
}

void
stack_push(const hstack_t hstack, const void* data_in, const unsigned int size)
{
    if (!stack_valid_handler(hstack))
        return;

    if (data_in == NULL || size == 0)
        return;

    // выделяем память для нового узла
    stack_t ptr = malloc(sizeof(*ptr) + size);
    if (ptr == NULL)
        return;

    // сдвигаем последний узел, изменяем его размер, а также копируем данные
    // (memcpy)
    stack_t curr = g_table.entries[hstack].stack;
    ptr->prev = curr;
    ptr->size = size;
    memcpy(ptr->data, data_in, size);

    // меняем голову(handler) стека
    g_table.entries[hstack].stack = ptr;
}

unsigned int
stack_pop(const hstack_t hstack, void* data_out, const unsigned int size)
{
    if (!(stack_valid_handler(hstack)))
        return 0;

    if (data_out == NULL || size == 0)
        return 0;

    // указатель на верхушку стека
    stack_t curr = g_table.entries[hstack].stack;

    if (curr == NULL)
        return 0;

    if (size < curr->size)
        return 0;

    // копируем в переменную байты значения узла
    unsigned int bytes_count = curr->size;
    memcpy(data_out, curr->data, bytes_count);

    // удаляем последний узел
    stack_t prev = (stack_t)curr->prev;
    g_table.entries[hstack].stack = prev;
    free(curr);

    return bytes_count;
}

unsigned stack_top_size(const hstack_t hstack)
{
    if (!(stack_valid_handler(hstack)))
        return 0;

    stack_t curr = g_table.entries[hstack].stack;

    // если стек пуст вернет 0, иначе его размер
    return (curr != NULL) ? curr->size : 0u;
}

hstack_t stack_get_all(void)
{
    hstack_t handler = stack_new();

    // если не получилось создать новый стек - метод падает с ошибкой
    if (handler == (hstack_t)-1)
        return (hstack_t)-1;

    for (size_t i = 0u; i < g_table.size; ++i)
    {
        if (g_table.entries[i].reserved == 1)
        {
            hstack_t current_handler = (hstack_t)i;
            // пропускаем результирующий узел
            if (current_handler == handler)
                continue;
            stack_push(handler, &current_handler, sizeof(current_handler));
        }
    }
    return handler;
}
