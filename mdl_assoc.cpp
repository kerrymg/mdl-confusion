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
#include <gc/gc.h>

#include <cstring>

#include <unistd.h>

#include "macros.hpp"
#include "mdl_assoc.hpp"
#include "mdl_internal_defs.h"

// associations are special, they need to disappear when their item or indicator
// disappears, but until then, they hold a reference to their value
// rather than futz with typed objects, I simply hold the key in a separately
// allocated structure

//#define MDL_ASSOC_NBUCKETS 101 // same as what SGI uses as STL default (first prime >= 100)
#define MDL_ASSOC_NBUCKETS 293

static inline
bool mdl_assoc_key_equals(const mdl_assoc_key_t &a, const mdl_assoc_key_t &b)
{
    return mdl_value_double_equal(a.item, b.item) &&
           mdl_value_double_equal(a.indicator, b.indicator);
}

static size_t mdl_hash_assoc_key(const mdl_assoc_key_t &h)
{
    size_t hitem = mdl_hash_value(h.item);
    size_t hindic = mdl_hash_value(h.indicator);
    size_t tmp;
    swab(&hindic, &tmp, sizeof(size_t));
//    std::printf("Hash: %lx %lx %lx %lx\n", hitem, hindic, tmp, hitem + tmp);
    return hitem + tmp;
}

mdl_assoc_table_t *
mdl_create_assoc_table()
{
    mdl_assoc_table_t *result = (mdl_assoc_table_t *)GC_MALLOC(sizeof(mdl_assoc_table_t) + sizeof(mdl_assoc_t *) * (MDL_ASSOC_NBUCKETS - 1));
    result->nbuckets = (MDL_ASSOC_NBUCKETS - 1);
    result->last_clean = GC_get_gc_no();
    return result;
}

void mdl_clear_assoc_table(mdl_assoc_table_t *table)
{
    // Garbage collection does make some things easier...
    memset(table->buckets, 0, sizeof(table->buckets[0]) * table->nbuckets);
    table->last_clean = GC_get_gc_no();
    table->size = 0;
}

int mdl_swap_assoc_table(mdl_assoc_table_t *t1, mdl_assoc_table_t *t2)
{
    if (t1->nbuckets != t2->nbuckets) return -1;

    mdl_assoc_t **t1p = t1->buckets;
    mdl_assoc_t **t2p = t2->buckets;
    for (int i = 0; i < t1->nbuckets; i++)
    {
        std::swap(*t2p, *t1p);
        ++t1p;
        ++t2p;
    }

    std::swap(t2->size, t1->size);
    std::swap(t2->last_clean, t1->last_clean);

    return 0;
}

static mdl_assoc_t *mdl_find_assoc(mdl_assoc_table_t *table, const mdl_assoc_key_t &inkey)
{
    if (table->last_clean != GC_get_gc_no()) mdl_assoc_clean(table);
    int bucketnum = mdl_hash_assoc_key(inkey) % table->nbuckets;
    mdl_assoc_t *cursor = table->buckets[bucketnum];
    while (cursor)
    {
        if (mdl_assoc_key_equals(*cursor->key, inkey))
            return cursor;
        cursor = cursor->next;
    }
    return nullptr;
}

mdl_value_t *mdl_assoc_find_value(mdl_assoc_table_t *table, const mdl_assoc_key_t &inkey)
{
    mdl_assoc_t *assoc = mdl_find_assoc(table, inkey);
    if (!assoc) return nullptr;
    return assoc->value;
}

mdl_value_t *mdl_delete_assoc(mdl_assoc_table_t *table, const mdl_assoc_key_t &inkey)
{
    if (table->last_clean != GC_get_gc_no()) mdl_assoc_clean(table);
    int bucketnum = mdl_hash_assoc_key(inkey) % table->nbuckets;
    mdl_assoc_t *cursor = table->buckets[bucketnum];
    if (!cursor) return nullptr; // no bucket means there was no such association
    // 1st item is special
    if (mdl_assoc_key_equals(*cursor->key, inkey))
    {
        table->buckets[bucketnum] = cursor->next;
        table->size--;
        return cursor->value;
    }
    mdl_assoc_t *lastcursor = cursor;
    cursor = cursor->next;

    while (cursor)
    {
        if (mdl_assoc_key_equals(*cursor->key, inkey))
        {
            lastcursor->next = cursor->next;
            table->size--;
            return cursor->value;
        }
        lastcursor = cursor;
        cursor = cursor->next;
    }
    return nullptr;
}

bool mdl_add_assoc(mdl_assoc_table_t *table, const mdl_assoc_key_t &inkey, mdl_value_t *value)
{
    mdl_assoc_t *assoc = mdl_find_assoc(table, inkey);
    if (!assoc)
    {
        mdl_assoc_key_t *key = GC_NEW_ATOMIC(mdl_assoc_key_t);
        *key = inkey;
        assoc = GC_NEW(mdl_assoc_t);
        assoc->key = key;
        assoc->value = value;
        assoc->item_exists = (void *)1;//&key->item;
        assoc->indicator_exists = (void *)1;//&key->indicator;
        GC_GENERAL_REGISTER_DISAPPEARING_LINK(&assoc->item_exists, key->item);
        GC_GENERAL_REGISTER_DISAPPEARING_LINK(&assoc->indicator_exists, key->indicator);
        int bucketnum = mdl_hash_assoc_key(*key) % table->nbuckets;
        assoc->next = table->buckets[bucketnum];
        table->buckets[bucketnum] = assoc;
        table->size++;
        return true;
    }
    else
    {
        assoc->value = value;
        return false;
    }
}

bool mdl_assoc_clean(mdl_assoc_table_t *table)
{
    mdl_assoc_iterator_t *iter = mdl_assoc_iterator_first(table);
    bool result = false;

//    std::fprintf(stderr, "ASSOC cleaning %d %p %lu %lu\n", table->size, table, table->last_clean, GC_gc_no);
    while (!mdl_assoc_iterator_at_end(iter))
    {
        if (!iter->assoc->item_exists || !iter->assoc->indicator_exists)
        {
//            std::fprintf(stderr, "Nuking an association\n");
            mdl_assoc_iterator_delete(iter);
            result = true;
        }
        else
        {
            mdl_assoc_iterator_increment(iter);
        }
    }
    table->last_clean = GC_get_gc_no();
//    std::fprintf(stderr, "ASSOC cleaning done %d\n", table->size);
    return result;
}

mdl_assoc_iterator_t *mdl_assoc_iterator_first(mdl_assoc_table_t *table)
{
    mdl_assoc_iterator_t *iter = GC_NEW(mdl_assoc_iterator_t);

    for (int i = 0; i < table->nbuckets; i++)
    {
        if ((iter->assoc = table->buckets[i]) != nullptr)
        {
            iter->bucket = i;
            break;
        }
    }
    iter->table = table;
    return iter;
}

bool mdl_assoc_iterator_increment(mdl_assoc_iterator_t *iter)
{
    if (iter->assoc == nullptr) return false; // iter's already dead, dude
    iter->assoc = iter->assoc->next;
    while (iter->assoc == nullptr && (++iter->bucket < iter->table->nbuckets))
    {
        iter->assoc = iter->table->buckets[iter->bucket];
    }
    return true;
}

// delete an element pointed to by iterator, and also advance iterator
// so it remains valid.  This delete works even if the key objects
// have been GCed.
bool mdl_assoc_iterator_delete(mdl_assoc_iterator_t *iter)
{
    bool result = false;

    if (iter->assoc == nullptr) return false; // iter's already dead, dude

    mdl_assoc_t *cursor = iter->table->buckets[iter->bucket];
    // 1st item is special
    if (cursor == iter->assoc)
    {
        iter->table->buckets[iter->bucket] = cursor->next;
        result = true;
        iter->table->size--;
    }
    else
    {
        mdl_assoc_t *lastcursor = cursor;
        cursor = cursor->next;

        while (cursor)
        {
            if (cursor == iter->assoc)
            {
                lastcursor->next = cursor->next;
                iter->table->size--;
                result = true;
                break;
            }
            lastcursor = cursor;
            cursor = cursor->next;
        }
    }
    // increment still works because it doesn't require iter to actually
    // exist in the table
    mdl_assoc_iterator_increment(iter);
    return result;
}
