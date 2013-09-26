/*
	SObjectizer 5.
*/

#include <iostream>

#include <cpp_util_2/h/lexcast.hpp>

#include <ace/Guard_T.h>
#include <ace/Thread_Manager.h>

#include <so_5/h/log_err.hpp>

#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/h/event_exception_handler.hpp>
#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

namespace so_5
{

namespace disp
{

namespace reuse
{

namespace work_thread
{

//
// demand_queue
//
demand_queue_t::demand_queue_t()
	:
		m_not_empty( m_lock )
{
}

demand_queue_t::~demand_queue_t()
{
	m_demands.clear();
}

void
demand_queue_t::push(
	const so_5::rt::agent_ref_t & agent_ref,
	unsigned int event_cnt )
{
	ACE_Guard< ACE_Thread_Mutex > lock( m_lock );

	if( m_in_service )
	{
		const bool demands_empty_before_service = m_demands.empty();

		// Assume that the new demand should be added to the queue.
		bool create_new_demand_needed = true;

		// Is queue was not empty...
		if( !demands_empty_before_service )
		{
			// ...and the last demand in the queue is for the same agent...
			demand_t & dmnd = m_demands.back();
			if( agent_ref.get() == dmnd.m_agent_ref.get() )
			{
				// ... then only the demand counter should be incremented.
				dmnd.m_event_cnt += event_cnt;
				create_new_demand_needed = false;
			}
		}

		if( create_new_demand_needed )
		{
			m_demands.push_back( demand_t( agent_ref, event_cnt ) );
		}

		if( demands_empty_before_service )
		{
			// May be someone is waiting...
			// It should be informed about new demands.
			m_not_empty.signal();
		}
	}
}

int
demand_queue_t::pop(
	demand_container_t & demands )
{
	ACE_Guard< ACE_Thread_Mutex > lock( m_lock );

	while( true )
	{
		if( m_in_service && !m_demands.empty() )
		{
			demands.swap( m_demands );
			break;
		}
		else if( !m_in_service )
			return shutting_down;
		else
		{
			// Queue is empty. We should wait for a demand or
			// a shutdown signal.
			m_not_empty.wait();
		}
	}

	return demand_extracted;
}

void
demand_queue_t::start_service()
{
	ACE_Guard< ACE_Thread_Mutex > lock( m_lock );

	m_in_service = true;
}

void
demand_queue_t::stop_service()
{
	bool is_someone_waiting_us = false;
	{
		ACE_Guard< ACE_Thread_Mutex > lock( m_lock );

		m_in_service = false;
		// If the demands queue is empty then someone is waiting
		// for new demands inside pop().
		is_someone_waiting_us = m_demands.empty();
	}

	// In case if someone is waiting.
	if( is_someone_waiting_us )
		m_not_empty.signal();
}

void
demand_queue_t::clear()
{
	ACE_Guard< ACE_Thread_Mutex > lock( m_lock );
	m_demands.clear();
}

//
// work_thread_t
//

work_thread_t::work_thread_t(
	rt::dispatcher_t & disp )
	:
		m_disp( disp )
{
}

work_thread_t::~work_thread_t()
{
}

void
work_thread_t::put_event_execution_request(
	const so_5::rt::agent_ref_t & agent_ref,
	unsigned int event_count )
{
	// Demands queue should serve this call.
	m_queue.push( agent_ref, event_count );
}

void
work_thread_t::start()
{
	m_queue.start_service();
	m_continue_work = WORK_THREAD_CONTINUE;

	SO_5_ABORT_ON_ACE_ERROR(
		ACE_Thread_Manager::instance()->spawn(
			entry_point,
			this,
			THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
			&m_tid ) );
}

void
work_thread_t::shutdown()
{
	m_continue_work = WORK_THREAD_STOP;
	m_queue.stop_service();
}

void
work_thread_t::wait()
{
	ACE_Thread_Manager::instance()->join( m_tid );

	m_queue.clear();
}

void
work_thread_t::body()
{
	// Local demands queue.
	demand_container_t demands;

	int result = demand_queue_t::no_demands;

	while( m_continue_work.value() == WORK_THREAD_CONTINUE )
	{
		try
		{
			while( m_continue_work.value() == WORK_THREAD_CONTINUE )
			{
				// If the local queue is empty then we should try
				// to get new demands.
				if( demands.empty() )
					result = m_queue.pop( demands );

				// Serve demands if any.
				if( demand_queue_t::demand_extracted == result )
					serve_demands_block( demands );
			}
		}
		catch( const std::exception & ex )
		{
			// Exception should be processed.
//FIXME: is it possible that the exception would be 
// caught when demands is empty?
			handle_exception(
				ex,
				demands.front().m_agent_ref );
		}
	}
}

void
work_thread_t::handle_exception(
	const std::exception & ex,
	const so_5::rt::agent_ref_t & a_exception_producer )
{
	auto response_action =
			m_disp.query_disp_evt_except_handler().handle_exception(
				ex,
				a_exception_producer->so_coop_name() );

	response_action->respond_to_exception();
}

inline void
work_thread_t::serve_demands_block(
	demand_container_t & demands )
{
	size_t demands_size_counter = demands.size();
	while( demands_size_counter-- )
	{
		demand_t & demand = demands.front();
		while( demand.m_event_cnt-- )
		{
			so_5::rt::agent_t::call_next_event(
				*demand.m_agent_ref );
		}
		demands.pop_front();
	}
}

ACE_THR_FUNC_RETURN
work_thread_t::entry_point( void * self_object )
{
	work_thread_t * work_thread =
		reinterpret_cast<work_thread_t *>( self_object );

	work_thread->body();

	return 0;
}

} /* namespace work_thread */

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

