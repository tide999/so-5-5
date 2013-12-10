/*
 * A test for checking so_5::rt::ignore_exception behaviour.
 */

#include <iostream>
#include <stdexcept>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

struct msg_test_signal : public so_5::rt::signal_t {};

class a_test_t
	:	public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_test_t(
			so_5::rt::so_environment_t & env )
			:	base_type_t( env )
			,	m_self_mbox( env.create_local_mbox() )
			,	m_counter( 0 )
			,	m_max_attempts( 3 )
		{}

		void
		so_define_agent()
		{
			so_subscribe( m_self_mbox ).event( &a_test_t::evt_signal );
		}

		virtual void
		so_evt_start()
		{
			m_self_mbox->deliver_signal< msg_test_signal >();
		}

		void
		evt_signal( const so_5::rt::event_data_t< msg_test_signal > & evt )
		{
			if( m_counter < m_max_attempts )
			{
				m_counter += 1;
				m_self_mbox->deliver_signal< msg_test_signal >();

				throw std::runtime_error( "Another exception from evt_signal" );
			}
			else
				so_environment().stop();
		}

		virtual so_5::rt::exception_reaction_t
		so_exception_reaction() const
		{
			return so_5::rt::ignore_exception;
		}

	private :
		const so_5::rt::mbox_ref_t m_self_mbox;

		int m_counter;
		const int m_max_attempts;
};

void
init( so_5::rt::so_environment_t & env )
{
	env.register_agent_as_coop( "test", new a_test_t( env ) );
}

int
main( int, char ** )
{
	try
	{
		so_5::api::run_so_environment( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
