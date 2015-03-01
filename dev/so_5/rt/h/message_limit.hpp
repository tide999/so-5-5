/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.4
 * \brief Public part of message limit implementation.
 */

#pragma once

#include <functional>
#include <typeindex>
#include <atomic>

#include <so_5/rt/h/message.hpp>

namespace so_5
{

namespace rt
{

namespace message_limit
{

//
// action_t
//
using action_t = std::function< void(const message_t*) >;

//
// drop_indicator_t
//
template< class M >
struct drop_indicator_t
	{
		//! Max count of waiting messages.
		const unsigned int m_limit;

		drop_indicator_t( unsigned int limit )
			:	m_limit( limit )
			{}
	};

//
// abort_app_indicator_t
//
template< class M >
struct abort_app_indicator_t
	{
		//! Max count of waiting messages.
		const unsigned int m_limit;

//FIXME: there should be reference to SOEnv.
//This reference is necessary for logging inside action.

		abort_app_indicator_t( unsigned int limit )
			:	m_limit( limit )
			{}
	};

//
// one_limit_describer_t
//
template< class M >
struct one_limit_describer_t
	{
		const drop_indicator_t< M > drop;
		const abort_app_indicator_t< M > abort_app;

		one_limit_describer_t( unsigned int limit )
			:	drop( limit )
			,	abort_app( limit )
			{}
	};

//
// description_t
//
struct description_t
	{
		//! Type of message.
		std::type_index m_msg_type;

		//! Max count of waiting messages.
		unsigned int m_limit;	

		//! Reaction to overload.
		action_t m_action;

		//! Initializing constructor.
		description_t(
			std::type_index msg_type,
			unsigned int limit,
			action_t action )
			:	m_msg_type( std::move( msg_type ) )
			,	m_limit( limit )
			,	m_action( std::move( action ) )
			{}
	};

//
// description_container_t
//
using description_container_t = std::vector< description_t >;

//
// accept_one_indicator
//
template< class M >
void
accept_one_indicator(
	description_container_t & to,
	const drop_indicator_t< M > & indicator )
	{
		to.emplace_back( typeid( M ),
				indicator.m_limit,
				[]( const message_t * ) {} );
	}

template< class M >
void
accept_one_indicator(
	description_container_t & to,
	const abort_app_indicator_t< M > & indicator )
	{
//FIXME: there must be an action with some logging.
		to.emplace_back( typeid( M ),
				indicator.m_limit,
				[]( const message_t * ) { std::abort(); } );
	}

//
// accept_indicators
//
template< typename I, typename... ARGS >
void
accept_indicators(
	description_container_t & to,
	I && indicator,
	ARGS &&... others )
	{
		accept_one_indicator( to, std::forward< I >( indicator ) );

		accept_indicators( to, std::forward< ARGS >( others )... );
	}

inline void
accept_indicators(
	description_container_t & to )
	{
	}

//
// control_block_t
//
/*!
 * \since v.5.5.4
 * \brief A control block for one message limit.
 */
struct control_block_t
	{
		//! Limit value.
		unsigned int m_limit;

		//! The current count of the messages of that type.
		mutable std::atomic_uint m_count;

		//! Limit overflow reaction.
		action_t m_action;

		//! Initializing constructor.
		control_block_t(
			unsigned int limit,
			action_t action )
			:	m_limit( limit )
			,	m_action( std::move( action ) )
			{
				m_count = 0;
			}
	};

namespace impl
{

// Forward declaration.
class info_storage_t;

} /* namespace impl */

} /* namespace message_limit */

} /* namespace rt */

} /* namespace so_5 */

