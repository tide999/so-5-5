/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief A multi-producer/single-consumer mbox definition.
*/

#if !defined( _SO_5__RT__IMPL__MPSC_MBOX_ )
#define _SO_5__RT__IMPL__MPSC_MBOX_

#include <so_5/h/types.hpp>
#include <so_5/h/exception.hpp>

#include <so_5/rt/h/mbox.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// mpsc_mbox_t
//

/*!
 * \since v.5.4.0
 * \brief A multi-producer/single-consumer mbox definition.
 */
class mpsc_mbox_t : public mbox_t
{
	public:
		explicit mpsc_mbox_t(
			mbox_id_t id,
			agent_t * single_consumer );

		virtual ~mpsc_mbox_t();

		virtual mbox_id_t
		id() const
			{
				return m_id;
			}

		virtual void
		deliver_service_request(
			const std::type_index & msg_type,
			const message_ref_t & svc_request_ref ) const;

		virtual void
		subscribe_event_handler(
			const std::type_index & msg_type,
			agent_t * subscriber );

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & msg_type,
			agent_t * subscriber );

//FIXME: It should return something like <mpsc:ID>
//instead of empty string.
		/*!
		 * \return Empty string.
		 */
		virtual const std::string &
		query_name() const;

	protected:
		void
		deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message_ref ) const;

	private:
		/*!
		 * \brief ID of this mbox.
		 */
		const mbox_id_t m_id;

		//! The only consumer of this mbox's messages.
		agent_t * m_single_consumer;
};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif
