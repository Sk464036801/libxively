#include "xi_heap.h"

xi_heap_t* xi_heap_create( xi_heap_index_type_t capacity )
{
    // PRECONDITIONS
    assert( capacity != 0 );

    size_t elements_size = sizeof( xi_heap_element_t* ) * capacity;

    xi_heap_t* ret = ( xi_heap_t* ) xi_alloc( sizeof( xi_heap_t ) );

    XI_CHECK_MEMORY( ret );

    ret->elements = ( xi_heap_element_t** ) xi_alloc( elements_size );

    XI_CHECK_MEMORY( ret->elements );

    memset( ret->elements, 0, elements_size );

    for( size_t i = 0; i < capacity; ++i )
    {
        ret->elements[ i ] = ( xi_heap_element_t* ) xi_alloc( sizeof( xi_heap_element_t ) );
        XI_CHECK_MEMORY( ret->elements );
        memset( ret->elements[ i ], 0, sizeof( xi_heap_element_t ) );
    }

    ret->capacity       = capacity;
    ret->first_free     = 0;

    // POSTCONDITIONS
    assert( ret->elements != 0 );
    assert( ret != 0 );

    return ret;

err_handling:
    if( ret->elements )
    {
        for( size_t i = 0; i < capacity; ++i )
        {
            XI_SAFE_FREE( ret->elements[ i ] );
        }
    }
    XI_SAFE_FREE( ret->elements );
    XI_SAFE_FREE( ret );
    return 0;
}

void xi_heap_destroy( xi_heap_t* xi_heap )
{
    // PRECONDITIONS
    assert( xi_heap != 0 );
    assert( xi_heap->elements != 0 );
    assert( xi_heap->capacity != 0 );

    if( xi_heap->elements )
    {
        for( size_t i = 0; i < xi_heap->capacity; ++i )
        {
            XI_SAFE_FREE( xi_heap->elements[ i ] );
        }
    }

    XI_SAFE_FREE( xi_heap->elements );
    XI_SAFE_FREE( xi_heap );

    // POSTONDITIONS
    assert( xi_heap == 0 );
}

// inxex calculus
// note: the indexes are increased and decreased
// in order to maintain the 0 - based indexing of elements
#define LEFT( i )   ( (   ( i + 1 ) << 1 ) - 1 )
#define RIGHT( i )  ( ( ( ( i + 1 ) << 1 ) + 1 ) - 1 )
#define PARENT( i ) (   ( ( i + 1 ) >> 1 ) - 1 )

void xi_heap_elements_swap( xi_heap_element_t** a, xi_heap_element_t** b )
{
    // PRECONDITIONS
    assert( a != 0 );
    assert( b != 0 );

    // swap elements
    xi_heap_element_t* tmp_elem = *a;
    *a = *b;
    *b = tmp_elem;

    // swap indexes
    xi_heap_index_type_t tmp_index = ( **a ).index;
    ( **a ).index = ( **b ).index;
    ( **b ).index = tmp_index;
}

void xi_heap_fix_order_up( xi_heap_t* xi_heap, xi_heap_index_type_t index )
{
    // PRECONDITIONS
    assert( xi_heap != 0 );
    assert( xi_heap->elements != 0 );
    assert( xi_heap->capacity != 0 );

    while( index != 0 )
    {
        xi_heap_element_t** e = &xi_heap->elements[ index ];
        xi_heap_element_t** p = &xi_heap->elements[ PARENT( index ) ];

        if( ( *e )->key < ( *p )->key )
        {
            xi_heap_elements_swap( e, p );
        }

        index = PARENT( index );
    }
}

void xi_heap_fix_order_down( xi_heap_t* xi_heap, xi_heap_index_type_t index )
{
    // PRECONDITIONS
    assert( xi_heap != 0 );
    assert( xi_heap->elements != 0 );
    assert( xi_heap->capacity != 0 );

    xi_heap_index_type_t li       = LEFT( index );
    xi_heap_index_type_t ri       = RIGHT( index );

    // guard the array bounds
    li = li >= xi_heap->first_free ? index : li;
    ri = ri >= xi_heap->first_free ? index : ri;

    do
    {
        xi_heap_element_t** e   = &xi_heap->elements[ index ];

        xi_heap_element_t** lc  = &xi_heap->elements[ li ];
        xi_heap_element_t** rc  = &xi_heap->elements[ ri ];

        if( ( *lc )->key < ( *rc )->key )
        {
            if( ( *lc )->key < ( *e )->key )
            {
                xi_heap_elements_swap( e, lc );
                index = li;
            }
            else { return; }
        }
        else
        {
            if( ( *rc )->key < ( *e )->key )
            {
                xi_heap_elements_swap( e, rc );
                index = ri;
            }
            else { return; }
        }

        // update the indexes
        li                      = LEFT( index );
        ri                      = RIGHT( index );

        // guard the array bounds
        li = li >= xi_heap->first_free ? index : li;
        ri = ri >= xi_heap->first_free ? index : ri;

    } while( index != li && index != ri );
}

xi_heap_element_t* xi_heap_element_add_void(
      xi_heap_t* xi_heap
    , xi_heap_key_type_t key
    , void* value )
{
    // PRECONDITIONS
    assert( xi_heap != 0 );
    assert( xi_heap->elements != 0 );
    assert( xi_heap->capacity != 0 );

    // check the capacity
    if( xi_heap->first_free > xi_heap->capacity ) { return 0; }

    // derefence
    xi_heap_element_t* element  = xi_heap->elements[ xi_heap->first_free ];

    // add the element
    element->index                      = xi_heap->first_free;
    element->key                        = key;
    element->heap_value.void_value      = value;

    // increase the next free counter
    xi_heap->first_free        += 1;

    // fix the order up
    xi_heap_fix_order_up( xi_heap, element->index );

    return element;
}

xi_heap_element_t* xi_heap_element_add(
      xi_heap_t* xi_heap
    , xi_heap_key_type_t key
    , xi_heap_value_type_t value )
{
    // PRECONDITIONS
    assert( xi_heap != 0 );
    assert( xi_heap->elements != 0 );
    assert( xi_heap->capacity != 0 );

    // check the capacity
    if( xi_heap->first_free > xi_heap->capacity ) { return 0; }

    // derefence
    xi_heap_element_t* element  = xi_heap->elements[ xi_heap->first_free ];

    // add the element
    element->index                      = xi_heap->first_free;
    element->key                        = key;
    element->heap_value.type_value      = value;

    // increase the next free counter
    xi_heap->first_free        += 1;

    // fix the order up
    xi_heap_fix_order_up( xi_heap, element->index );

    return element;
}

void xi_heap_element_remove(
      xi_heap_t* xi_heap
    , xi_heap_element_t* elem )
{
    assert( xi_heap != 0 );
    assert( elem != 0 );

    xi_heap_index_type_t last_i = xi_heap->first_free - 1;

    if( last_i != elem->index )
    {
        xi_heap_element_t** last_e  = &xi_heap->elements[ last_i ];
        xi_heap_element_t** e       = &xi_heap->elements[ elem->index ];
        xi_heap_index_type_t e_i    = ( *e )->index;

        xi_heap_elements_swap( e, last_e );
        xi_heap_fix_order_down( xi_heap, e_i );
        xi_heap_fix_order_up( xi_heap, e_i );
    }

    xi_heap->first_free -= 1;
}

void xi_heap_element_update_key(
      xi_heap_t* xi_heap
    , xi_heap_element_t* elem
    , xi_heap_key_type_t new_key )
{
    assert( xi_heap != 0 );
    assert( elem != 0 );

    elem->key = new_key;

    xi_heap_element_t** e       = &xi_heap->elements[ elem->index ];
    xi_heap_index_type_t e_i    = ( *e )->index;

    xi_heap_fix_order_down( xi_heap, e_i );
    xi_heap_fix_order_up( xi_heap, e_i );
}

xi_heap_element_t* xi_heap_get_top( xi_heap_t* xi_heap )
{
    // PRECONDITIONS
    assert( xi_heap != 0 );
    assert( xi_heap->elements != 0 );
    assert( xi_heap->capacity != 0 );

    if( xi_heap->first_free == 0 )
    {
        return 0;
    }

    xi_heap_element_t** last_e  = 0;
    xi_heap_element_t** e       = &xi_heap->elements[ 0 ];
    xi_heap_index_type_t last_i = xi_heap->first_free - 1;
    xi_heap->first_free        -= 1;

    if( last_i != 0 )
    {
        last_e = &xi_heap->elements[ last_i ];
        xi_heap_elements_swap( e, last_e );
        xi_heap_fix_order_down( xi_heap, 0 );
        return *last_e;
    }

    return *e;
}

xi_heap_element_t* xi_heap_peek_top( xi_heap_t* xi_heap )
{
    return xi_heap->elements[ 0 ];
}

uint8_t xi_heap_is_empty( xi_heap_t* xi_heap )
{
    return xi_heap->first_free == 0 ? 1 : 0;
}
