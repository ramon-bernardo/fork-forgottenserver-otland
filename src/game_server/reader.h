#pragma once

#include "incoming_message.h"
#include "connection.h"

#include <memory>

namespace tfs::game_server {

class Reader
{
public:
	constexpr Reader() = default;
	virtual ~Reader() = default;

private:
	void read_add_vip(IncomingMessage& msg);
	void read_auto_walk(IncomingMessage& msg);
	void read_attack(IncomingMessage& msg);
	void read_browse_field(IncomingMessage& msg);
	void read_channel_exclude(IncomingMessage& msg);
	void read_channel_invite(IncomingMessage& msg);
	void read_close_channel(IncomingMessage& msg);
	void read_close_container(IncomingMessage& msg);
	void read_debug_assert(IncomingMessage& msg);
	void read_edit_podium_request(IncomingMessage& msg);
	void read_edit_vip(IncomingMessage& msg);
	void read_enable_shared_party_experience(IncomingMessage& msg);
	void read_equip_object(IncomingMessage& msg);
	void read_fight_modes(IncomingMessage& msg);
	void read_follow(IncomingMessage& msg);
	void read_house_window(IncomingMessage& msg);
	void read_invite_to_party(IncomingMessage& msg);
	void read_join_party(IncomingMessage& msg);
	void read_look_at(IncomingMessage& msg);
	void read_look_in_battle_list(IncomingMessage& msg);
	void read_look_in_shop(IncomingMessage& msg);
	void read_look_in_trade(IncomingMessage& msg);
	void read_market_accept_offer(IncomingMessage& msg);
	void read_market_browse(IncomingMessage& msg);
	void read_market_cancel_offer(IncomingMessage& msg);
	void read_market_create_offer(IncomingMessage& msg);
	void read_market_leave();
	void read_modal_window_answer(IncomingMessage& msg);
	void read_open_channel(IncomingMessage& msg);
	void read_open_private_channel(IncomingMessage& msg);
	void read_pass_party_leadership(IncomingMessage& msg);
	void read_player_purchase(IncomingMessage& msg);
	void read_player_sale(IncomingMessage& msg);
	void read_remove_vip(IncomingMessage& msg);
	void read_request_trade(IncomingMessage& msg);
	void read_revoke_party_invite(IncomingMessage& msg);
	void read_rotate_item(IncomingMessage& msg);
	void read_rule_violation_report(IncomingMessage& msg);
	void read_say(IncomingMessage& msg);
	void read_seek_in_container(IncomingMessage& msg);
	void read_set_outfit(IncomingMessage& msg);
	void read_text_window(IncomingMessage& msg);
	void read_throw(IncomingMessage& msg);
	void read_up_arrow_container(IncomingMessage& msg);
	void read_update_container(IncomingMessage& msg);
	void read_use_item(IncomingMessage& msg);
	void read_use_item_ex(IncomingMessage& msg);
	void read_use_with_creature(IncomingMessage& msg);
	void read_wrap_item(IncomingMessage& msg);

private:
	virtual uint32_t get_player_id() const = 0;
};

} // namespace tfs::game_server
