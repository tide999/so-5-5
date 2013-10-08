/*
 * A test for delayed message.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>

#include <ace/OS.h>
#include <ace/Time_Value.h>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

struct test_message
	: public so_5::rt::signal_t
{};

class test_agent_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:

		test_agent_t(
			so_5::rt::so_environment_t & env )
			:
				base_type_t( env ),
				m_test_mbox( so_environment().create_local_mbox() )
		{
		}

		virtual ~test_agent_t()
		{
		}

		virtual void
		so_define_agent();

		virtual void
		so_evt_start()
		{
			// Send a delayed for 1 second message.
			m_timer_ref = so_environment().schedule_timer< test_message >(
				m_test_mbox,
				1000,
				0 );
		}

		void
		evt_test(
			const so_5::rt::event_data_t< test_message > &
				msg );

	private:
		so_5::rt::mbox_ref_t	m_test_mbox;

		so_5::timer_thread::timer_id_ref_t	m_timer_ref;

};

void
test_agent_t::so_define_agent()
{
	so_subscribe( m_test_mbox )
		.event( &test_agent_t::evt_test );
}

void
test_agent_t::evt_test(
	const so_5::rt::event_data_t< test_message > &
		msg )
{
	so_environment().stop();
}

void
init( so_5::rt::so_environment_t & env )
{
	so_5::rt::agent_coop_unique_ptr_t coop =
		env.create_coop( "test_coop" );

	coop->add_agent( new test_agent_t( env ) );

	env.register_coop( std::move( coop ) );
}

int
main( int argc, char * argv[] )
{
	try
	{
		so_5::api::run_so_environment(
			&init,
			std::move(
				so_5::rt::so_environment_params_t()
					.mbox_mutex_pool_size( 4 )
					.agent_event_queue_mutex_pool_size( 4 ) ) );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

