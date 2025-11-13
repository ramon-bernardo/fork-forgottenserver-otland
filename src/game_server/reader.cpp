#include "reader.h"

#include "../game.h"
#include "../tasks.h"

extern Game g_game;

namespace tfs::game_server {

void Reader::read_add_vip(IncomingMessage& msg)
{
	const auto name = msg.get<std::string>();

	g_dispatcher.addTask([id = get_player_id(), name = std::string{name}]() { g_game.playerRequestAddVip(id, name); });
}

void Reader::read_auto_walk(IncomingMessage& msg)
{
	const auto directions = msg.get<uint8_t>();
	if (directions == 0 || (msg.position() + directions) != (msg.len() + 8)) {
		return;
	}

	msg.advance(directions);

	std::vector<Direction> path;
	path.reserve(directions);

	for (uint8_t i = 0; i < directions; ++i) {
		const auto raw = msg.get_prev<uint8_t>();
		switch (raw) {
			case 1:
				path.push_back(DIRECTION_EAST);
				break;
			case 2:
				path.push_back(DIRECTION_NORTHEAST);
				break;
			case 3:
				path.push_back(DIRECTION_NORTH);
				break;
			case 4:
				path.push_back(DIRECTION_NORTHWEST);
				break;
			case 5:
				path.push_back(DIRECTION_WEST);
				break;
			case 6:
				path.push_back(DIRECTION_SOUTHWEST);
				break;
			case 7:
				path.push_back(DIRECTION_SOUTH);
				break;
			case 8:
				path.push_back(DIRECTION_SOUTHEAST);
				break;
			default:
				break;
		}
	}

	if (path.empty()) {
		return;
	}

	g_dispatcher.addTask([id = get_player_id(), path = std::move(path)]() { g_game.playerAutoWalk(id, path); });
}

void Reader::read_attack(IncomingMessage& msg)
{
	const auto creature_id = msg.get<uint32_t>();
	// msg.get<uint32_t>(); creature_id (same as above)

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerSetAttackedCreature(id, creature_id); });
}

void Reader::read_browse_field(IncomingMessage& msg)
{
	const auto position = msg.get<Position>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerBrowseField(id, position); });
}

void Reader::read_channel_exclude(IncomingMessage& msg)
{
	const auto name = msg.get<std::string>();

	g_dispatcher.addTask(
	    [=, id = get_player_id(), name = std::string{name}]() { g_game.playerChannelExclude(id, name); });
}

void Reader::read_channel_invite(IncomingMessage& msg)
{
	const auto name = msg.get<std::string>();

	g_dispatcher.addTask([id = get_player_id(), name = std::string{name}]() { g_game.playerChannelInvite(id, name); });
}

void Reader::read_close_channel(IncomingMessage& msg)
{
	const auto channel_id = msg.get<uint16_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerCloseChannel(id, channel_id); });
}

void Reader::read_close_container(IncomingMessage& msg)
{
	const auto container_id = msg.get<uint8_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerCloseContainer(id, container_id); });
}

void Reader::read_debug_assert(IncomingMessage& msg)
{
	const auto assert_line = msg.get<std::string>();
	const auto date = msg.get<std::string>();
	const auto description = msg.get<std::string>();
	const auto comment = msg.get<std::string>();

	g_dispatcher.addTask([id = get_player_id(), assert_line = std::string{assert_line}, date = std::string{date},
	                      description = std::string{description}, comment = std::string{comment}]() {
		g_game.playerDebugAssert(id, assert_line, date, description, comment);
	});
}

void Reader::read_edit_podium_request(IncomingMessage& msg)
{
	const auto position = msg.get<Position>();
	const auto sprite_id = msg.get<uint16_t>();
	const auto stack_pos = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
		g_game.playerRequestEditPodium(id, position, stack_pos, sprite_id);
	});
}

void Reader::read_edit_vip(IncomingMessage& msg)
{
	const auto guid = msg.get<uint32_t>();
	const auto description = msg.get<std::string>();
	const auto icon = std::min<uint32_t>(10, msg.get<uint32_t>()); // 10 is max icon in 9.63
	const auto notify = msg.get<uint8_t>() != 0;

	g_dispatcher.addTask([=, id = get_player_id(), description = std::string{description}]() {
		g_game.playerRequestEditVip(id, guid, description, icon, notify);
	});
}

void Reader::read_enable_shared_party_experience(IncomingMessage& msg)
{
	const auto active = msg.get<uint8_t>() == 1;

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerEnableSharedPartyExperience(id, active); });
}

void Reader::read_equip_object(IncomingMessage& msg)
{
	// hotkey equip (?)
	const auto sprite_id = msg.get<uint16_t>();
	// msg.get<uint8_t>(); // bool smartMode (?)

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerEquipItem(id, sprite_id); });
}

void Reader::read_fight_modes(IncomingMessage& msg)
{
	const auto raw_fight_mode = msg.get<uint8_t>();  // 1 - offensive, 2 - balanced, 3 - defensive
	const auto raw_chase_mode = msg.get<uint8_t>();  // 0 - stand while fighting, 1 - chase opponent
	const auto raw_secure_mode = msg.get<uint8_t>(); // 0 - can't attack unmarked, 1 - can attack unmarked
	// const auto raw_pvp_mode = msg.get<uint8_t>(); // pvp mode introduced in 10.0

	fightMode_t fight_mode;
	if (raw_fight_mode == 1) {
		fight_mode = FIGHTMODE_ATTACK;
	} else if (raw_fight_mode == 2) {
		fight_mode = FIGHTMODE_BALANCED;
	} else {
		fight_mode = FIGHTMODE_DEFENSE;
	}

	g_dispatcher.addTask([=, id = get_player_id()]() {
		g_game.playerSetFightModes(id, fight_mode, raw_chase_mode != 0, raw_secure_mode != 0);
	});
}

void Reader::read_follow(IncomingMessage& msg)
{
	const auto creature_id = msg.get<uint32_t>();
	// msg.get<uint32_t>(); creatureID (same as above)

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerFollowCreature(id, creature_id); });
}

void Reader::read_house_window(IncomingMessage& msg)
{
	const auto door_id = msg.get<uint8_t>();
	const auto window_id = msg.get<uint32_t>();
	const auto text = msg.get<std::string>();

	g_dispatcher.addTask([=, id = get_player_id(), text = std::string{text}]() {
		g_game.playerUpdateHouseWindow(id, door_id, window_id, text);
	});
}

void Reader::read_invite_to_party(IncomingMessage& msg)
{
	const auto target_id = msg.get<uint32_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerInviteToParty(id, target_id); });
}

void Reader::read_join_party(IncomingMessage& msg)
{
	const auto target_id = msg.get<uint32_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerJoinParty(id, target_id); });
}

void Reader::read_look_at(IncomingMessage& msg)
{
	const auto position = msg.get<Position>();
	msg.advance(2); // sprite_id
	const auto stack_pos = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerLookAt(id, position, stack_pos); });
}

void Reader::read_look_in_battle_list(IncomingMessage& msg)
{
	const auto creature_id = msg.get<uint32_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerLookInBattleList(id, creature_id); });
}

void Reader::read_look_in_shop(IncomingMessage& msg)
{
	const auto shop_id = msg.get<uint16_t>();
	const auto count = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerLookInShop(id, shop_id, count); });
}

void Reader::read_look_in_trade(IncomingMessage& msg)
{
	const auto counter_offer = msg.get<uint8_t>() == 0x01;
	const auto index = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerLookInTrade(id, counter_offer, index); });
}

void Reader::read_market_accept_offer(IncomingMessage& msg)
{
	const auto timestamp = msg.get<uint32_t>();
	const auto counter = msg.get<uint16_t>();
	const auto amount = msg.get<uint16_t>();

	g_dispatcher.addTask(
	    [=, id = get_player_id()]() { g_game.playerAcceptMarketOffer(id, timestamp, counter, amount); });
}

void Reader::read_market_browse(IncomingMessage& msg)
{
	const auto browse_id = msg.get<uint8_t>();
	if (browse_id == MARKETREQUEST_OWN_OFFERS) {
		g_dispatcher.addTask([id = get_player_id()]() { g_game.playerBrowseMarketOwnOffers(id); });
	} else if (browse_id == MARKETREQUEST_OWN_HISTORY) {
		g_dispatcher.addTask([id = get_player_id()]() { g_game.playerBrowseMarketOwnHistory(id); });
	} else {
		const auto sprite_id = msg.get<uint16_t>();
		g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerBrowseMarket(id, sprite_id); });
	}
}

void Reader::read_market_cancel_offer(IncomingMessage& msg)
{
	const auto timestamp = msg.get<uint32_t>();
	const auto counter = msg.get<uint16_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerCancelMarketOffer(id, timestamp, counter); });
}

void Reader::read_market_create_offer(IncomingMessage& msg)
{
	const auto type = msg.get<uint8_t>();
	const auto sprite_id = msg.get<uint16_t>();

	const ItemType& it = Item::items.getItemIdByClientId(sprite_id);
	if (it.id == 0 || it.wareId == 0) {
		return;
	} else if (it.classification > 0) {
		msg.get<uint8_t>(); // item tier
	}

	const auto amount = msg.get<uint16_t>();
	const auto price = msg.get<uint64_t>();
	const auto anonymous = msg.get<uint8_t>() != 0;

	g_dispatcher.addTask(
	    [=, id = get_player_id()]() { g_game.playerCreateMarketOffer(id, type, sprite_id, amount, price, anonymous); });
}

void Reader::read_market_leave(IncomingMessage& msg)
{
	g_dispatcher.addTask([id = get_player_id()]() { g_game.playerLeaveMarket(id); });
}

void Reader::read_modal_window_answer(IncomingMessage& msg)
{
	const auto window_id = msg.get<uint32_t>();
	const auto button = msg.get<uint8_t>();
	const auto choice = msg.get<uint8_t>();

	g_dispatcher.addTask(
	    [=, id = get_player_id()]() { g_game.playerAnswerModalWindow(id, window_id, button, choice); });
}

void Reader::read_open_channel(IncomingMessage& msg)
{
	const auto channel_id = msg.get<uint16_t>();
	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerOpenChannel(id, channel_id); });
}

void Reader::read_open_private_channel(IncomingMessage& msg)
{
	const auto receiver = msg.get<std::string>();
	g_dispatcher.addTask(
	    [id = get_player_id(), receiver = std::string{receiver}]() { g_game.playerOpenPrivateChannel(id, receiver); });
}

void Reader::read_pass_party_leadership(IncomingMessage& msg)
{
	const auto target_id = msg.get<uint32_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerPassPartyLeadership(id, target_id); });
}

void Reader::read_player_purchase(IncomingMessage& msg)
{
	const auto shop_id = msg.get<uint16_t>();
	const auto count = msg.get<uint8_t>();
	const auto amount = msg.get<uint16_t>();
	const auto ignore_cap = msg.get<uint8_t>() != 0;
	const auto in_backpacks = msg.get<uint8_t>() != 0;

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
		g_game.playerPurchaseItem(id, shop_id, count, amount, ignore_cap, in_backpacks);
	});
}

void Reader::read_player_sale(IncomingMessage& msg)
{
	const auto shop_id = msg.get<uint16_t>();
	const auto count = msg.get<uint8_t>();
	const auto amount = msg.get<uint16_t>();
	const auto ignore_equipped = msg.get<uint8_t>() != 0;

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
		g_game.playerSellItem(id, shop_id, count, amount, ignore_equipped);
	});
}

void Reader::read_remove_vip(IncomingMessage& msg)
{
	const auto guid = msg.get<uint32_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerRequestRemoveVip(id, guid); });
}

void Reader::read_request_trade(IncomingMessage& msg)
{
	const auto position = msg.get<Position>();
	const auto sprite_id = msg.get<uint16_t>();
	const auto stack_pos = msg.get<uint8_t>();
	const auto target_id = msg.get<uint32_t>();

	g_dispatcher.addTask(
	    [=, id = get_player_id()]() { g_game.playerRequestTrade(id, position, stack_pos, target_id, sprite_id); });
}

void Reader::read_look_in_trade(IncomingMessage& msg)
{
	const auto counter_offer = msg.get<uint8_t>() == 0x01;
	const auto index = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerLookInTrade(id, counter_offer, index); });
}

void Reader::read_revoke_party_invite(IncomingMessage& msg)
{
	const auto target_id = msg.get<uint32_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerRevokePartyInvitation(id, target_id); });
}

void Reader::read_rotate_item(IncomingMessage& msg)
{
	const auto position = msg.get<Position>();
	const auto sprite_id = msg.get<uint16_t>();
	const auto stack_pos = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerRotateItem(id, position, stack_pos, sprite_id); });
}

void Reader::read_rule_violation_report(IncomingMessage& msg)
{
	const auto type = msg.get<uint8_t>();
	const auto reason = msg.get<uint8_t>();
	const auto target_name = msg.get<std::string>();
	const auto comment = msg.get<std::string>();

	std::string translation;
	if (type == REPORT_TYPE_NAME) {
		translation = msg.get<std::string>();
	} else if (type == REPORT_TYPE_STATEMENT) {
		translation = msg.get<std::string>();
		msg.get<uint32_t>(); // statement id, used to get whatever player have said, we don't log that.
	}

	g_dispatcher.addTask([=, id = get_player_id(), target_name = std::string{target_name},
	                      comment = std::string{comment}, translation = std::string{translation}]() {
		g_game.playerReportRuleViolation(id, target_name, type, reason, comment, translation);
	});
}

void Reader::read_say(IncomingMessage& msg)
{
	std::string receiver;
	auto channel_id = 0;

	const auto type = static_cast<SpeakClasses>(msg.get<uint8_t>());
	switch (type) {
		case TALKTYPE_PRIVATE_TO:
		case TALKTYPE_PRIVATE_RED_TO:
			receiver = msg.get<std::string>();
			break;

		case TALKTYPE_CHANNEL_Y:
		case TALKTYPE_CHANNEL_R1:
			channel_id = msg.get<uint16_t>();
			break;

		default:
			break;
	}

	const auto text = msg.get<std::string>();
	if (text.length() > 255) {
		return;
	}

	g_dispatcher.addTask([=, id = get_player_id(), receiver = std::string{receiver}, text = std::string{text}]() {
		g_game.playerSay(id, channel_id, type, receiver, text);
	});
}

void Reader::read_seek_in_container(IncomingMessage& msg)
{
	const auto container_id = msg.get<uint8_t>();
	const auto index = msg.get<uint16_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerSeekInContainer(id, container_id, index); });
}

void Reader::read_set_outfit(IncomingMessage& msg)
{
	const auto type = msg.get<uint8_t>();

	Outfit_t outfit;
	outfit.lookType = msg.get<uint16_t>();
	outfit.lookHead = msg.get<uint8_t>();
	outfit.lookBody = msg.get<uint8_t>();
	outfit.lookLegs = msg.get<uint8_t>();
	outfit.lookFeet = msg.get<uint8_t>();
	outfit.lookAddons = msg.get<uint8_t>();

	if (type == 0) {
		outfit.lookMount = msg.get<uint16_t>();
		if (outfit.lookMount != 0) {
			outfit.lookMountHead = msg.get<uint8_t>();
			outfit.lookMountBody = msg.get<uint8_t>();
			outfit.lookMountLegs = msg.get<uint8_t>();
			outfit.lookMountFeet = msg.get<uint8_t>();
		} else {
			msg.advance(4);

			if (const auto player = g_game.getPlayerByID(get_player_id())) {
				const Outfit_t& currentOutfit = player->getCurrentOutfit();
				outfit.lookMountHead = currentOutfit.lookMountHead;
				outfit.lookMountBody = currentOutfit.lookMountBody;
				outfit.lookMountLegs = currentOutfit.lookMountLegs;
				outfit.lookMountFeet = currentOutfit.lookMountFeet;
			}
		}

		msg.get<uint16_t>();
		const auto randomize_mount = msg.get<uint8_t>() == 0x01;

		g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerChangeOutfit(id, outfit, randomize_mount); });
	} else if (type == 1) {
		outfit.lookMount = 0;
		outfit.lookMountHead = msg.get<uint8_t>();
		outfit.lookMountBody = msg.get<uint8_t>();
		outfit.lookMountLegs = msg.get<uint8_t>();
		outfit.lookMountFeet = msg.get<uint8_t>();
	} else if (type == 2) {
		const auto pos = msg.get<Position>();
		const auto sprite_id = msg.get<uint16_t>();
		const auto stack_pos = msg.get<uint8_t>();

		outfit.lookMount = msg.get<uint16_t>();
		outfit.lookMountHead = msg.get<uint8_t>();
		outfit.lookMountBody = msg.get<uint8_t>();
		outfit.lookMountLegs = msg.get<uint8_t>();
		outfit.lookMountFeet = msg.get<uint8_t>();
		const auto direction = static_cast<Direction>(msg.get<uint8_t>());
		const auto podium_visible = msg.get<uint8_t>() == 1;

		g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
			g_game.playerEditPodium(id, outfit, pos, stack_pos, sprite_id, podium_visible, direction);
		});
	}
}

void Reader::read_text_window(IncomingMessage& msg)
{
	const auto window_id = msg.get<uint32_t>();
	const auto text = msg.get<std::string>();

	g_dispatcher.addTask([id = get_player_id(), window_id, text]() { g_game.playerWriteItem(id, window_id, text); });
}

void Reader::read_throw(IncomingMessage& msg)
{
	const auto from_position = msg.get<Position>();
	const auto sprite_id = msg.get<uint16_t>();
	const auto from_stack_pos = msg.get<uint8_t>();
	const auto to_position = msg.get<Position>();
	const auto count = msg.get<uint8_t>();

	if (to_position != from_position) {
		g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
			g_game.playerMoveThing(id, from_position, sprite_id, from_stack_pos, to_position, count);
		});
	}
}

void Reader::read_up_arrow_container(IncomingMessage& msg)
{
	const auto container_id = msg.get<uint8_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerMoveUpContainer(id, container_id); });
}

void Reader::read_update_container(IncomingMessage& msg)
{
	const auto container_id = msg.get<uint8_t>();

	g_dispatcher.addTask([=, id = get_player_id()]() { g_game.playerUpdateContainer(id, container_id); });
}

void Reader::read_use_item(IncomingMessage& msg)
{
	const auto position = msg.get<Position>();
	const auto sprite_id = msg.get<uint16_t>();
	const auto stack_pos = msg.get<uint8_t>();
	const auto index = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
		g_game.playerUseItem(id, position, stack_pos, index, sprite_id);
	});
}

void Reader::read_use_item_ex(IncomingMessage& msg)
{
	const auto from_position = msg.get<Position>();
	const auto from_sprite_id = msg.get<uint16_t>();
	const auto from_stack_pos = msg.get<uint8_t>();
	const auto to_position = msg.get<Position>();
	const auto to_sprite_id = msg.get<uint16_t>();
	const auto to_stack_pos = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
		g_game.playerUseItemEx(id, from_position, from_stack_pos, from_sprite_id, to_position, to_stack_pos,
		                       to_sprite_id);
	});
}

void Reader::read_use_with_creature(IncomingMessage& msg)
{
	const auto from_position = msg.get<Position>();
	const auto sprite_id = msg.get<uint16_t>();
	const auto from_stack_pos = msg.get<uint8_t>();
	const auto creature_id = msg.get<uint32_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, id = get_player_id()]() {
		g_game.playerUseWithCreature(id, from_position, from_stack_pos, creature_id, sprite_id);
	});
}

void Reader::read_wrap_item(IncomingMessage& msg)
{
	const auto position = msg.get<Position>();
	const auto sprite_id = msg.get<uint16_t>();
	const auto stack_pos = msg.get<uint8_t>();

	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, id = get_player_id()]() { g_game.playerWrapItem(id, position, stack_pos, sprite_id); });
}

} // namespace tfs::game_server
