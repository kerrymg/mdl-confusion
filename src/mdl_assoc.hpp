/*****************************************************************************/
/*    'Confusion', a MDL intepreter                                         */
/*    Copyright 2009 Matthew T. Russotto                                    */
/*                                                                          */
/*    This program is free software: you can redistribute it and/or modify  */
/*    it under the terms of the GNU General Public License as published by  */
/*    the Free Software Foundation, version 3 of 29 June 2007.              */
/*                                                                          */
/*    This program is distributed in the hope that it will be useful,       */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*    GNU General Public License for more details.                          */
/*                                                                          */
/*    You should have received a copy of the GNU General Public License     */
/*    along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*****************************************************************************/

#ifndef MDL_ASSOC_H_
#define MDL_ASSOC_H_

struct mdl_assoc_t
{
    struct mdl_assoc_t *next;
    struct mdl_assoc_key_t *key;
    mdl_value_t *value;
    void *item_exists;
    void *indicator_exists;
};

struct mdl_assoc_table_t
{
    int nbuckets;
    int size;
    GC_word last_clean;
    mdl_assoc_t *buckets[1];
};

struct mdl_assoc_iterator_t
{
    mdl_assoc_table_t *table;
    int bucket;
    mdl_assoc_t *assoc;
};

inline int
mdl_assoc_table_size(mdl_assoc_table_t *table)
{
    return table->size;
}

inline bool
mdl_assoc_iterator_at_end(const mdl_assoc_iterator_t *iter)
{
    return iter->assoc == nullptr;
}

inline const mdl_assoc_key_t *mdl_assoc_iterator_get_key(mdl_assoc_iterator_t *iter)
{
    if (!iter->assoc)
    {
        return nullptr;
    }
    return (const mdl_assoc_key_t *)iter->assoc->key;
}

inline mdl_value_t *mdl_assoc_iterator_get_value(mdl_assoc_iterator_t *iter)
{
    if (!iter->assoc)
    {
        return nullptr;
    }
    return iter->assoc->value;
}

mdl_assoc_table_t *mdl_create_assoc_table();
bool mdl_assoc_clean(mdl_assoc_table_t *table);
void mdl_clear_assoc_table(mdl_assoc_table_t *table);
int mdl_swap_assoc_table(mdl_assoc_table_t *t1, mdl_assoc_table_t *t2);

bool mdl_add_assoc(mdl_assoc_table_t *table, const mdl_assoc_key_t &inkey, mdl_value_t *value);
mdl_value_t *mdl_assoc_find_value(mdl_assoc_table_t *table, const mdl_assoc_key_t &inkey);

mdl_value_t *mdl_delete_assoc(mdl_assoc_table_t *table, const mdl_assoc_key_t &inkey);

// Iterator routines
mdl_assoc_iterator_t *mdl_assoc_iterator_first(mdl_assoc_table_t *table);
bool mdl_assoc_iterator_increment(mdl_assoc_iterator_t *iter);
bool mdl_assoc_iterator_delete(mdl_assoc_iterator_t *iter);

#endif // MDL_ASSOC_H_
