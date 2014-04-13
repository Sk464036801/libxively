// Copyright (c) 2003-2014, LogMeIn, Inc. All rights reserved.
// This is part of Xively C library, it is under the BSD 3-Clause license.

/**
 * \file    xively.c
 * \brief   Xively C library [see xively.h]
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xi_allocator.h"
#include "xively.h"
#include "xi_macros.h"
#include "xi_debug.h"
#include "xi_helpers.h"
#include "xi_err.h"
#include "xi_globals.h"
#include "xi_common.h"
#include "xi_layer_api.h"
#include "xi_layer_interface.h"
#include "xi_layer_connection.h"
#include "xi_layer_types_conf.h"
#include "xi_layer_factory.h"
#include "xi_layer_factory_conf.h"
#include "xi_layer_default_allocators.h"
#include "xi_connection_data.h"
#include "xi_static_vector.h"

#if defined( XI_MQTT_ENABLED ) && defined( XI_NOB_ENABLED )
#include "xi_event_dispatcher_global_instance.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------
// HELPER MACROS
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// HELPER FUNCTIONS
//-----------------------------------------------------------------------
#ifndef XI_MQTT_ENABLED

#include "xi_http_layer.h"
#include "xi_http_layer_data.h"
#include "xi_csv_layer.h"

xi_value_type_t xi_get_value_type( xi_datapoint_t* p )
{
    // PRECONDITION
    assert( p != 0 );

    return p->value_type;
}

xi_datapoint_t* xi_set_value_i32( xi_datapoint_t* p, int32_t value )
{
    // PRECONDITION
    assert( p != 0 );

    p->value.i32_value  = value;
    p->value_type       = XI_VALUE_TYPE_I32;

    return p;
}

int32_t xi_get_value_i32( xi_datapoint_t* p )
{
    // PRECONDITION
    assert( p != 0 );
    assert( p->value_type == XI_VALUE_TYPE_I32 );

    return p->value.i32_value;
}

int32_t* xi_value_pointer_i32( xi_datapoint_t* p )
{
    // PRECONDITION
    assert( p != 0 );

    if( p->value_type == XI_VALUE_TYPE_I32 )
    {
      return &p->value.i32_value;
    }
    else
    {
      return NULL;
    }
}

xi_datapoint_t* xi_set_value_f32( xi_datapoint_t* p, float value )
{
    // PRECONDITION
    assert( p != 0 );

    p->value.f32_value  = value;
    p->value_type       = XI_VALUE_TYPE_F32;

    return p;
}

float xi_get_value_f32( xi_datapoint_t* p )
{
    // PRECONDITION
    assert( p != 0 );
    assert( p->value_type == XI_VALUE_TYPE_F32 );

    if( p->value_type == XI_VALUE_TYPE_F32 )
    {
      return p->value.f32_value;
    }
    else
    {
      return 0.;
    }
}

float* xi_value_pointer_f32( xi_datapoint_t* p )
{
    // PRECONDITION
    assert( p != 0 );

    if( p->value_type == XI_VALUE_TYPE_F32 )
    {
      return &p->value.f32_value;
    }
    else
    {
      return NULL;
    }
}

xi_datapoint_t* xi_set_value_str( xi_datapoint_t* p, const char* value )
{
    // PRECONDITION
    assert( p != 0 );

    int s = xi_str_copy_untiln( p->value.str_value
        , XI_VALUE_STRING_MAX_SIZE, value, '\0' );

    XI_CHECK_SIZE( s, XI_VALUE_STRING_MAX_SIZE, XI_DATAPOINT_VALUE_BUFFER_OVERFLOW );

    p->value_type = XI_VALUE_TYPE_STR;

    return p;

err_handling:
    return 0;
}

char* xi_value_pointer_str( xi_datapoint_t* p )
{
    // PRECONDITION
    assert( p != 0 );

    if( p->value_type == XI_VALUE_TYPE_STR )
    {
      return p->value.str_value;
    }
    else
    {
      return NULL;
    }
}
#endif // XI_MQTT_ENABLED

void xi_set_network_timeout( uint32_t timeout )
{
    xi_globals.network_timeout = timeout;
}

uint32_t xi_get_network_timeout( void )
{
    return xi_globals.network_timeout;
}

//-----------------------------------------------------------------------
// LAYERS SETTINGS
//-----------------------------------------------------------------------

#define XI_IO_POSIX           0
#define XI_IO_DUMMY           1
#define XI_IO_MBED            2
#define XI_IO_POSIX_ASYNCH    3

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief The LAYERS_ID enum
///
enum LAYERS_ID
{
      IO_LAYER = 0
#ifndef XI_MQTT_ENABLED
    , HTTP_LAYER
    , CSV_LAYER
#else // XI_MQTT_ENABLED
    , MQTT_LAYER
    , MQTT_LOGIC_LAYER
#endif // XI_MQTT_ENABLED
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef XI_MQTT_ENABLED
#define CONNECTION_SCHEME_1_DATA IO_LAYER, HTTP_LAYER, CSV_LAYER
#else // XI_MQTT_ENABLED
#define CONNECTION_SCHEME_2_DATA IO_LAYER, MQTT_LAYER, MQTT_LOGIC_LAYER
#endif // XI_MQTT_ENABLED

#ifndef XI_MQTT_ENABLED
DEFINE_CONNECTION_SCHEME( CONNECTION_SCHEME_1, CONNECTION_SCHEME_1_DATA );
#else // XI_MQTT_ENABLED
DEFINE_CONNECTION_SCHEME( CONNECTION_SCHEME_2, CONNECTION_SCHEME_2_DATA );
#endif // XI_MQTT_ENABLED

#if XI_IO_LAYER == XI_IO_POSIX

    // posix io layer
    #include "posix_io_layer.h"

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    #ifdef XI_MQTT_ENABLED

    #include "xi_mqtt_layer.h"
    #include "xi_mqtt_layer_data.h"

    BEGIN_LAYER_TYPES_CONF()
          LAYER_TYPE( IO_LAYER, &posix_io_layer_data_ready, &posix_io_layer_on_data_ready
                                , &posix_io_layer_close, &posix_io_layer_on_close
                                , &posix_io_layer_init, &posix_io_layer_connect )
        , LAYER_TYPE( MQTT_LAYER, &xi_mqtt_layer_data_ready, &xi_mqtt_layer_on_data_ready
                                , &xi_mqtt_layer_close, &xi_mqtt_layer_on_close, 0, 0 )
    END_LAYER_TYPES_CONF()
    #else
    BEGIN_LAYER_TYPES_CONF()
          LAYER_TYPE( IO_LAYER, &posix_io_layer_data_ready, &posix_io_layer_on_data_ready
                                , &posix_io_layer_close, &posix_io_layer_on_close
                                , &posix_io_layer_init, &posix_io_layer_connect )
        , LAYER_TYPE( HTTP_LAYER, &http_layer_data_ready, &http_layer_on_data_ready
                                , &http_layer_close, &http_layer_on_close, 0, 0 )
        , LAYER_TYPE( CSV_LAYER, &csv_layer_data_ready, &csv_layer_on_data_ready
                            , &csv_layer_close, &csv_layer_on_close, 0, 0 )
    END_LAYER_TYPES_CONF()
    #endif

#elif XI_IO_LAYER == XI_IO_DUMMY
    // dummy io layer
    #include "dummy_io_layer.h"

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    BEGIN_LAYER_TYPES_CONF()
          LAYER_TYPE( IO_LAYER, &dummy_io_layer_data_ready, &dummy_io_layer_on_data_ready
                              , &dummy_io_layer_close, &dummy_io_layer_on_close
                              , &dummy_io_layer_init, &dummy_io_layer_connect )
        , LAYER_TYPE( HTTP_LAYER, &http_layer_data_ready, &http_layer_on_data_ready
                                , &http_layer_close, &http_layer_on_close, 0, 0 )
        , LAYER_TYPE( CSV_LAYER, &csv_layer_data_ready, &csv_layer_on_data_ready
                            , &csv_layer_close, &csv_layer_on_close, 0, 0 )
    END_LAYER_TYPES_CONF()

#elif XI_IO_LAYER == XI_IO_MBED
    // mbed io layer
    #include "mbed_io_layer.h"

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    #ifdef XI_MQTT_ENABLED

    #include "xi_mqtt_layer.h"
    #include "xi_mqtt_layer_data.h"

    BEGIN_LAYER_TYPES_CONF()
          LAYER_TYPE( IO_LAYER, &mbed_io_layer_data_ready, &mbed_io_layer_on_data_ready
                                , &mbed_io_layer_close, &mbed_io_layer_on_close
                                , &mbed_io_layer_init, &mbed_io_layer_connect )
        , LAYER_TYPE( MQTT_LAYER, &xi_mqtt_layer_data_ready, &xi_mqtt_layer_on_data_ready
                                , &xi_mqtt_layer_close, &xi_mqtt_layer_on_close, 0, 0 )
    END_LAYER_TYPES_CONF()
    #else
    BEGIN_LAYER_TYPES_CONF()
          LAYER_TYPE( IO_LAYER, &mbed_io_layer_data_ready, &mbed_io_layer_on_data_ready
                              , &mbed_io_layer_close, &mbed_io_layer_on_close
                              , &mbed_io_layer_init, &mbed_io_layer_connect )
        , LAYER_TYPE( HTTP_LAYER, &http_layer_data_ready, &http_layer_on_data_ready
                                , &http_layer_close, &http_layer_on_close, 0, 0 )
        , LAYER_TYPE( CSV_LAYER, &csv_layer_data_ready, &csv_layer_on_data_ready
                            , &csv_layer_close, &csv_layer_on_close, 0, 0 )
    END_LAYER_TYPES_CONF()
    #endif

#elif XI_IO_LAYER == XI_IO_POSIX_ASYNCH
    // mbed io layer
    #include "posix_asynch_io_layer.h"

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    #ifdef XI_MQTT_ENABLED

    #include "xi_mqtt_layer.h"
    #include "xi_mqtt_layer_data.h"
    #include "xi_mqtt_logic_layer.h"
    #include "xi_mqtt_logic_layer_data.h"

    BEGIN_LAYER_TYPES_CONF()
          LAYER_TYPE( IO_LAYER, &posix_asynch_io_layer_data_ready, &posix_asynch_io_layer_on_data_ready
                                , &posix_asynch_io_layer_close, &posix_asynch_io_layer_on_close
                                , &posix_asynch_io_layer_init, &posix_asynch_io_layer_connect )
        , LAYER_TYPE( MQTT_LAYER, &xi_mqtt_layer_data_ready, &xi_mqtt_layer_on_data_ready
                                , &xi_mqtt_layer_close, &xi_mqtt_layer_on_close
                                , &xi_mqtt_layer_init, &xi_mqtt_layer_connect )
        , LAYER_TYPE( MQTT_LOGIC_LAYER, &xi_mqtt_logic_layer_data_ready, &xi_mqtt_logic_layer_on_data_ready
                                , &xi_mqtt_logic_layer_close, &xi_mqtt_logic_layer_on_close
                                , &xi_mqtt_logic_layer_init, &xi_mqtt_logic_layer_connect )
    END_LAYER_TYPES_CONF()
    #else
    BEGIN_LAYER_TYPES_CONF()
          LAYER_TYPE( IO_LAYER, &posix_asynch_io_layer_data_ready, &posix_asynch_io_layer_on_data_ready
                              , &posix_asynch_io_layer_close, &posix_asynch_io_layer_on_close
                              , &posix_asynch_io_layer_init, &posix_asynch_io_layer_connect )
        , LAYER_TYPE( HTTP_LAYER, &http_layer_data_ready, &http_layer_on_data_ready
                                , &http_layer_close, &http_layer_on_close, 0, 0 )
        , LAYER_TYPE( CSV_LAYER, &csv_layer_data_ready, &csv_layer_on_data_ready
                            , &csv_layer_close, &csv_layer_on_close, 0, 0 )
    END_LAYER_TYPES_CONF()
    #endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_FACTORY_CONF()
      FACTORY_ENTRY( IO_LAYER, &placement_layer_pass_create, &placement_layer_pass_delete
                             , &default_layer_heap_alloc, &default_layer_heap_free )
#ifndef XI_MQTT_ENABLED
    , FACTORY_ENTRY( HTTP_LAYER, &placement_layer_pass_create, &placement_layer_pass_delete
                               , &default_layer_heap_alloc, &default_layer_heap_free )
    , FACTORY_ENTRY( CSV_LAYER, &placement_layer_pass_create, &placement_layer_pass_delete
                           , &default_layer_heap_alloc, &default_layer_heap_free )
#else // XI_MQTT_ENABLED
    , FACTORY_ENTRY( MQTT_LAYER, &placement_layer_pass_create, &placement_layer_pass_delete
                           , &default_layer_heap_alloc, &default_layer_heap_free )
    , FACTORY_ENTRY( MQTT_LOGIC_LAYER, &placement_layer_pass_create, &placement_layer_pass_delete
                           , &default_layer_heap_alloc, &default_layer_heap_free )
#endif // XI_MQTT_ENABLED
END_FACTORY_CONF()

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// MAIN LIBRARY FUNCTIONS
//-----------------------------------------------------------------------

xi_context_t* xi_create_context(
      xi_protocol_t protocol
    , const char* api_key
    , xi_feed_id_t feed_id )
{

#if defined( XI_MQTT_ENABLED ) && defined( XI_NOB_ENABLED )
    XI_UNUSED( api_key );
    XI_UNUSED( feed_id );

    if( xi_evtd_ref_count == 0 )
    {
        xi_evtd_instance = xi_evtd_create_instance();
        XI_CHECK_MEMORY( xi_evtd_instance );
    }
    else
    {
        xi_evtd_ref_count += 1;
    }
#endif

    // allocate the structure to store new context
    xi_context_t* ret = ( xi_context_t* ) xi_alloc( sizeof( xi_context_t ) );

    XI_CHECK_MEMORY( ret );

    // copy given numeric parameters as is
    ret->protocol       = protocol;
#ifndef XI_MQTT_ENABLED
    ret->feed_id        = feed_id;
    // copy string parameters carefully
    if( api_key )
    {
        // duplicate the string
        ret->api_key  = xi_str_dup( api_key );

        XI_CHECK_MEMORY( ret->api_key );
    }
    else
    {
        ret->api_key  = 0;
    }
#endif

    switch( protocol )
    {
        #ifndef XI_MQTT_ENABLED
        case XI_HTTP:
            {
                // @TODO make a configurable pool of these
                // static structures allocated statically
                static http_layer_data_t    http_layer_data;
                static csv_layer_data_t     csv_layer_data;
                static xi_response_t        xi_response;


                // clean the structures
                memset( &http_layer_data, 0, sizeof( http_layer_data_t ) );
                memset( &csv_layer_data, 0, sizeof( csv_layer_data_t ) );
                memset( &xi_response, 0, sizeof( xi_response_t ) );

                // the response pointer
                http_layer_data.response    = &xi_response;
                csv_layer_data.response     = &xi_response;

                // prepare user data description
                void* user_datas[] = { 0, ( void* ) &http_layer_data, ( void* ) &csv_layer_data };

                // create and connect layers store the information in layer_chain member
                ret->layer_chain = create_and_connect_layers( CONNECTION_SCHEME_1, user_datas, CONNECTION_SCHEME_LENGTH( CONNECTION_SCHEME_1 ) );
            }
            break;
        #else  // XI_MQTT_ENABLED
        case XI_MQTT:
            {
                void* user_datas[] = { 0, 0, 0 };
                ret->layer_chain = create_and_connect_layers( CONNECTION_SCHEME_2, user_datas, CONNECTION_SCHEME_LENGTH( CONNECTION_SCHEME_2 ) );
            }
            break;
        #endif // XI_MQTT_ENABLED
        default:
            goto err_handling;
    }

    return ret;

err_handling:
#ifndef XI_MQTT_ENABLED
    if( ret )
    {
        if( ret->api_key )
        {
            XI_SAFE_FREE( ret->api_key );
        }

        XI_SAFE_FREE( ret );
    }
#endif

    return 0;
}

void xi_delete_context( xi_context_t* context )
{
    assert( context != 0 && "context must not be null!" );

    switch( context->protocol )
    {
        #ifndef XI_MQTT_ENABLED
        case XI_HTTP:
            destroy_and_disconnect_layers( &( context->layer_chain ), CONNECTION_SCHEME_LENGTH( CONNECTION_SCHEME_1 ) );
            break;
        #else // XI_MQTT_ENABLED
        case XI_MQTT:
            destroy_and_disconnect_layers( &( context->layer_chain ), CONNECTION_SCHEME_LENGTH( CONNECTION_SCHEME_2 ) );
            break;
        #endif // XI_MQTT_ENABLED
        default:
            assert( 0 && "not yet implemented!" );
            break;
    }
#ifndef XI_MQTT_ENABLED
    XI_SAFE_FREE( context->api_key );
#endif
    XI_SAFE_FREE( context );

#if defined( XI_MQTT_ENABLED ) && defined( XI_NOB_ENABLED )
    xi_evtd_ref_count -= 1;

    if( xi_evtd_ref_count == 0 )
    {
        xi_evtd_destroy_instance( xi_evtd_instance );
    }
#endif

}

#if !defined(XI_MQTT_ENABLED) && !defined(XI_NOB_ENABLED)

const xi_response_t* xi_feed_get(
          xi_context_t* xi
        , xi_feed_t* feed )
{
    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_FEED_GET
        , xi
        , 0
        , { .xi_get_feed = { .feed = feed } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}

const xi_response_t* xi_feed_get_all(
          xi_context_t* xi
        , xi_feed_t* feed )
{
    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_FEED_GET_ALL
        , xi
        , 0
        , { .xi_get_feed = { .feed = feed } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}


const xi_response_t* xi_feed_update(
          xi_context_t* xi
        , const xi_feed_t* feed )
{
    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_FEED_UPDATE
        , xi
        , 0
        , { .xi_update_feed = { ( xi_feed_t * ) feed } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}

const xi_response_t* xi_datastream_get(
            xi_context_t* xi, xi_feed_id_t feed_id
          , const char * datastream_id, xi_datapoint_t* o )
{
    XI_UNUSED( feed_id );

    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_DATASTREAM_GET
        , xi
        , 0
        , { ( struct xi_get_datastream_t ) { datastream_id, o } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}


const xi_response_t* xi_datastream_create(
            xi_context_t* xi, xi_feed_id_t feed_id
          , const char * datastream_id
          , const xi_datapoint_t* datapoint )
{
    XI_UNUSED( feed_id );

    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_DATASTREAM_CREATE
        , xi
        , 0
        , { .xi_create_datastream = { ( char* ) datastream_id, ( xi_datapoint_t* ) datapoint } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}

const xi_response_t* xi_datastream_update(
          xi_context_t* xi, xi_feed_id_t feed_id
        , const char * datastream_id
        , const xi_datapoint_t* datapoint )
{
    XI_UNUSED( feed_id );

    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_DATASTREAM_UPDATE
        , xi
        , 0
        , { .xi_update_datastream = { ( char* ) datastream_id, ( xi_datapoint_t* ) datapoint } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}

const xi_response_t* xi_datastream_delete(
            xi_context_t* xi, xi_feed_id_t feed_id
          , const char * datastream_id )
{
    XI_UNUSED( feed_id );

    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_DATASTREAM_DELETE
        , xi
        , 0
        , { .xi_delete_datastream = { datastream_id } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}

const xi_response_t* xi_datapoint_delete(
          const xi_context_t* xi, xi_feed_id_t feed_id
        , const char * datastream_id
        , const xi_datapoint_t* o )
{
    XI_UNUSED( feed_id );

    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_DATAPOINT_DELETE
        , ( xi_context_t* ) xi
        , 0
        , { .xi_delete_datapoint = { ( char* ) datastream_id, ( xi_datapoint_t* ) o } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void * ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}

extern const xi_response_t* xi_datapoint_delete_range(
            const xi_context_t* xi, xi_feed_id_t feed_id
          , const char * datastream_id
          , const xi_timestamp_t* start
          , const xi_timestamp_t* end )
{
    XI_UNUSED( feed_id );

    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_STATE_OK );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    http_layer_input_t http_layer_input =
    {
          HTTP_LAYER_INPUT_DATAPOINT_DELETE_RANGE
        , ( xi_context_t* ) xi
        , 0
        , { .xi_delete_datapoint_range = { ( char* ) datastream_id, ( xi_timestamp_t* ) start, ( xi_timestamp_t* ) end } }
    };

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &http_layer_input, LAYER_STATE_OK );

    if( state == LAYER_STATE_OK )
    {
        CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_STATE_OK );
    }

    CALL_ON_SELF_CLOSE( input_layer, ( void* ) 0, LAYER_STATE_OK );

    return ( ( csv_layer_data_t* ) input_layer->user_data )->response;
}

#elif !defined(XI_MQTT_ENABLED) && defined(XI_NOB_ENABLED)

extern const xi_context_t* xi_nob_feed_update(
         xi_context_t* xi
       , const xi_feed_t* value )
{
    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type = HTTP_LAYER_INPUT_FEED_UPDATE;
    http_layer_input.xi_context = xi;
    http_layer_input.http_union_data.xi_update_feed.feed = value;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

extern const xi_context_t* xi_nob_feed_get(
         xi_context_t* xi
       , xi_feed_t* value )
{
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }
    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type = HTTP_LAYER_INPUT_FEED_GET;
    http_layer_input.xi_context = xi;
    http_layer_input.http_union_data.xi_get_feed.feed = value;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

extern const xi_context_t* xi_nob_feed_get_all(
          xi_context_t* xi
        , xi_feed_t* value )
{
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type = HTTP_LAYER_INPUT_FEED_GET_ALL;
    http_layer_input.xi_context = xi;
    http_layer_input.http_union_data.xi_get_feed.feed = value;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}


extern const xi_context_t* xi_nob_datastream_create(
         xi_context_t* xi, xi_feed_id_t feed_id
       , const char * datastream_id
       , const xi_datapoint_t* value )
{
    XI_UNUSED( feed_id );

    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type                                         = HTTP_LAYER_INPUT_DATASTREAM_CREATE;
    http_layer_input.xi_context                                         = xi;
    http_layer_input.http_union_data.xi_create_datastream.datastream    = datastream_id;
    http_layer_input.http_union_data.xi_create_datastream.value         = value;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

extern const xi_context_t* xi_nob_datastream_update(
         xi_context_t* xi, xi_feed_id_t feed_id
       , const char * datastream_id
       , const xi_datapoint_t* value )
{
    XI_UNUSED( feed_id );

    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type                                         = HTTP_LAYER_INPUT_DATASTREAM_UPDATE;
    http_layer_input.xi_context                                         = xi;
    http_layer_input.http_union_data.xi_update_datastream.datastream    = datastream_id;
    http_layer_input.http_union_data.xi_update_datastream.value         = value;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

const xi_context_t* xi_nob_datastream_get(
         xi_context_t* xi, xi_feed_id_t feed_id
       , const char * datastream_id, xi_datapoint_t* dp )
{
    XI_UNUSED( feed_id );

    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type = HTTP_LAYER_INPUT_DATASTREAM_GET;
    http_layer_input.xi_context = xi;
    http_layer_input.http_union_data.xi_get_datastream.datastream = datastream_id;
    http_layer_input.http_union_data.xi_get_datastream.value = dp;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

const xi_context_t* xi_nob_datastream_delete(
         xi_context_t* xi, xi_feed_id_t feed_id
       , const char* datastream_id )
{
    XI_UNUSED( feed_id );

    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type                                         = HTTP_LAYER_INPUT_DATASTREAM_DELETE;
    http_layer_input.xi_context                                         = xi;
    http_layer_input.http_union_data.xi_delete_datastream.datastream    = datastream_id;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

const xi_context_t* xi_nob_datapoint_delete(
         xi_context_t* xi, xi_feed_id_t feed_id
       , const char * datastream_id
       , const xi_datapoint_t* dp )
{
    XI_UNUSED( feed_id );

    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type                                         = HTTP_LAYER_INPUT_DATAPOINT_DELETE;
    http_layer_input.xi_context                                         = xi;
    http_layer_input.http_union_data.xi_delete_datapoint.datastream     = datastream_id;
    http_layer_input.http_union_data.xi_delete_datapoint.value          = dp;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

const xi_context_t* xi_nob_datapoint_delete_range(
        xi_context_t* xi, xi_feed_id_t feed_id, const char * datastream_id
      , const xi_timestamp_t* start, const xi_timestamp_t* end )
{
    XI_UNUSED( feed_id );

    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
    if( state != LAYER_STATE_OK ) { return 0; }

    // clean the response before writing to it
    memset( ( ( csv_layer_data_t* ) input_layer->user_data )->response, 0, sizeof( xi_response_t ) );

    // create the input parameter
    static http_layer_input_t http_layer_input;
    memset( &http_layer_input, 0, sizeof( http_layer_input_t ) );

    // set the layer input
    http_layer_input.query_type                                             = HTTP_LAYER_INPUT_DATAPOINT_DELETE_RANGE;
    http_layer_input.xi_context                                             = xi;
    http_layer_input.http_union_data.xi_delete_datapoint_range.datastream   = datastream_id;
    http_layer_input.http_union_data.xi_delete_datapoint_range.value_start  = start;
    http_layer_input.http_union_data.xi_delete_datapoint_range.value_end    = end;

    // assign the input parameter so that can be used via the runner
    xi->input = &http_layer_input;

    return xi;
}

#elif defined(XI_MQTT_ENABLED) && !defined(XI_NOB_ENABLED)

#include "message.h"

extern const xi_response_t* xi_mqtt_publish(
      xi_context_t* xi
    , const char* topic
    , const char* msg )
{
    // we shall need it later
    layer_state_t state = LAYER_STATE_OK;

    // extract the input layer
    layer_t* input_layer    = xi->layer_chain.top;
    layer_t* io_layer       = xi->layer_chain.bottom;

    { // init & connect
        state = CALL_ON_SELF_INIT( io_layer, 0, LAYER_HINT_NONE );
        if( state != LAYER_STATE_OK ) { return 0; }

        xi_connection_data_t conn_data = { XI_HOST, XI_PORT };

        state = CALL_ON_SELF_CONNECT( io_layer, ( void *) &conn_data, LAYER_HINT_NONE );
        if( state != LAYER_STATE_OK ) { return 0; }
    }

    // first connect
    mqtt_message_t message;
    memset( &message, 0, sizeof( mqtt_message_t ) );

    message.common.common_u.common_bits.retain     = MQTT_RETAIN_FALSE;
    message.common.common_u.common_bits.qos        = MQTT_QOS_AT_MOST_ONCE;
    message.common.common_u.common_bits.dup        = MQTT_DUP_FALSE;
    message.common.common_u.common_bits.type       = MQTT_TYPE_CONNECT;
    message.common.remaining_length                = 0; // ?????????

    memcpy( message.connect.protocol_name.data, "MQIsdp", 6 );
    message.connect.protocol_name.length                    = 6;
    message.connect.protocol_version                        = 3;

    message.connect.flags_u.flags_bits.username_follows     = 0;
    message.connect.flags_u.flags_bits.password_follows     = 0;
    message.connect.flags_u.flags_bits.will_retain          = 0;
    message.connect.flags_u.flags_bits.will_qos             = 0;
    message.connect.flags_u.flags_bits.will                 = 0;
    message.connect.flags_u.flags_bits.clean_session        = 0;

    message.connect.keep_alive                              = 0;

    {
        const char client_id[] = "xi_test_client";
        message.connect.client_id.length = sizeof( client_id ) - 1;
        memcpy( message.connect.client_id.data, client_id, sizeof( client_id ) - 1 );
    }

    state = CALL_ON_SELF_DATA_READY( input_layer, ( void *) &message, LAYER_HINT_NONE );

    if( state == LAYER_STATE_OK )
    {
        state = CALL_ON_SELF_ON_DATA_READY( io_layer, ( void *) 0, LAYER_HINT_NONE );
    }

    xi_mqtt_layer_data_t* test = ( xi_mqtt_layer_data_t* ) input_layer->user_data;
    mqtt_message_dump( &test->msg );

    message.common.common_u.common_bits.type    = MQTT_TYPE_PUBLISH;
    message.publish.topic_name.length           = strlen( topic );
    message.publish.content.length              = strlen( msg );

    memcpy(
          message.publish.topic_name.data
        , topic
        , message.publish.topic_name.length );

    memcpy(
          message.publish.content.data
        , msg
        , message.publish.content.length );

    if( state == LAYER_STATE_OK )
    {
        state = CALL_ON_SELF_DATA_READY( input_layer, ( void* )&message, LAYER_HINT_NONE );
    }

    CALL_ON_SELF_CLOSE( input_layer );

    return 0;
}

#elif defined(XI_MQTT_ENABLED) && defined(XI_NOB_ENABLED)

extern layer_state_t xi_nob_mqtt_connect(
      xi_context_t* xi
    , xi_connection_data_t* connection_data
    , xi_evtd_handle_t on_connected )
{
    layer_t* input_layer = xi->layer_chain.top;

    connection_data->on_connected = on_connected;

    return CALL_ON_SELF_INIT( &input_layer->layer_connection
        , connection_data
        , LAYER_STATE_OK );
}

extern layer_state_t xi_nob_mqtt_publish(
      xi_context_t* xi
    , const char* topic
    , const char* msg )
{
    XI_UNUSED( xi );
    XI_UNUSED( topic );
    XI_UNUSED( msg );

    layer_t* input_layer = xi->layer_chain.top;

    xi_mqtt_logic_layer_data_t* layer_data
        = ( xi_mqtt_logic_layer_data_t* ) input_layer->user_data;

    xi_mqtt_logic_task_t* task
        = ( xi_mqtt_logic_task_t* ) xi_alloc( sizeof( xi_mqtt_logic_task_t ) );
    XI_CHECK_MEMORY( task );
    memset( task, 0, sizeof( xi_mqtt_logic_task_t ) );

    task->data.mqtt_settings.scenario_t = XI_MQTT_PUBLISH;
    task->data.mqtt_settings.qos_t      = XI_MQTT_QOS_ZERO;
    task->data.data_u = ( union data_t* ) xi_alloc( sizeof( union data_t ) );
    XI_CHECK_MEMORY( task->data.data_u );
    task->data.data_u->publish.topic     = xi_str_dup( topic );
    XI_CHECK_MEMORY( task->data.data_u->publish.topic );
    task->data.data_u->publish.msg       = xi_str_dup( msg );
    XI_CHECK_MEMORY( task->data.data_u->publish.msg );

    return CALL_ON_SELF_DATA_READY(
          &input_layer->layer_connection
        , task
        , LAYER_STATE_OK );

err_handling:
    if( task )
    {
        if( task->data.data_u )
        {
            XI_SAFE_FREE( task->data.data_u->publish.msg );
            XI_SAFE_FREE( task->data.data_u->publish.topic );
            XI_SAFE_FREE( task );
        }
        XI_SAFE_FREE( task->data.data_u );
    }
    return LAYER_STATE_ERROR;
}

layer_state_t xi_nob_mqtt_subscribe(
      xi_context_t* xi
    , const char* topic
    , xi_evtd_handle_t handler )
{
    layer_t* input_layer = xi->layer_chain.top;

    xi_mqtt_logic_layer_data_t* layer_data
        = ( xi_mqtt_logic_layer_data_t* ) input_layer->user_data;

    xi_mqtt_logic_task_t* task
        = ( xi_mqtt_logic_task_t* ) xi_alloc( sizeof( xi_mqtt_logic_task_t ) );
    XI_CHECK_MEMORY( task );
    memset( task, 0, sizeof( xi_mqtt_logic_task_t ) );

    task->data.mqtt_settings.scenario_t = XI_MQTT_SUBSCRIBE;
    task->data.mqtt_settings.qos_t      = XI_MQTT_QOS_ONE;
    task->data.data_u = ( union data_t* ) xi_alloc( sizeof( union data_t ) );
    XI_CHECK_MEMORY( task->data.data_u );
    task->data.data_u->subscribe.topic  = xi_str_dup( topic );
    XI_CHECK_MEMORY( task->data.data_u->subscribe.topic );
    task->data.data_u->subscribe.handler= handler;

    xi_static_vector_push(
            layer_data->handlers_for_topics
          , task->data.data_u );

    return CALL_ON_SELF_DATA_READY(
              &input_layer->layer_connection
            , task
            , LAYER_STATE_OK
        );

err_handling:
    if( task )
    {
        if( task->data.data_u )
        {
            XI_SAFE_FREE( task->data.data_u->subscribe.topic );
            XI_SAFE_FREE( task->data.data_u );
        }
        XI_SAFE_FREE( task );
    }
    return LAYER_STATE_ERROR;
}


#endif

#ifdef __cplusplus
}
#endif
