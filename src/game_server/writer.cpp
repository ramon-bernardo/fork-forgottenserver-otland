#include "writer.h"

namespace tfs::game_server {

void Writer::write_login_error(std::string_view error)
{
	msg.add<uint8_t>(0x14);
	msg.add(error);
	enqueue(msg.take());
}

void Writer::write_add_container_item(uint8_t cid, uint16_t slot, const Item* item)
{
	msg.add<uint8_t>(0x70);
	msg.add<uint8_t>(cid);
	msg.add<uint16_t>(slot);
	msg.add(item);
	enqueue(msg.take());
}

void Writer::write_add_creature(const Creature* creature, const Position& pos, int32_t stackpos,
                                MagicEffectClasses magic_effect)
{
	if (!canSee(pos)) {
		return;
	}

	if (creature != player) {
		// stack pos is always real index now, so it can exceed the limit if stack pos exceeds the limit, we need to
		// refresh the tile instead
		// 1. this is a rare case, and is only triggered by forcing summon in a position
		// 2. since no stackpos will be send to the client about that creature, removing it must be done with its id if
		// its stackpos remains >= MAX_STACKPOS. this is done to add creatures to battle list instead of rendering on
		// screen
		if (stackpos >= MAX_STACKPOS) {
			// @todo: should we avoid this check?
			if (const Tile* tile = creature->getTile()) {
				sendUpdateTile(tile, pos);
			}
		} else {
			// if stackpos is -1, the client will automatically detect it
			NetworkMessage msg;
			msg.add<uint8_t>(0x6A);
			msg.addPosition(pos);
			msg.add<uint8_t>(stackpos);

			bool known;
			uint32_t removedKnown;
			checkCreatureAsKnown(creature->getID(), known, removedKnown);
			AddCreature(msg, creature, known, removedKnown);
			enqueue(msg.take());
		}

		if (magicEffect != CONST_ME_NONE) {
			sendMagicEffect(pos, magicEffect);
		}
		return;
	}

	// send player stats
	sendStats();  // hp, cap, level, xp rate, etc.
	sendSkills(); // skills and special skills
	sendIcons();  // active conditions

	// send client info
	sendClientFeatures(); // player speed, bug reports, store url, pvp mode, etc
	sendBasicData();      // premium account, vocation, known spells, prey system status, magic shield status
	sendItems();          // send carried items for action bars

	// enter world and send game screen
	sendPendingStateEntered();
	sendEnterWorld();
	sendMapDescription(pos);

	// send login effect
	if (magicEffect != CONST_ME_NONE) {
		sendMagicEffect(pos, magicEffect);
	}

	// send equipment
	for (int i = CONST_SLOT_FIRST; i <= CONST_SLOT_LAST; ++i) {
		sendInventoryItem(static_cast<slots_t>(i), player->getInventoryItem(static_cast<slots_t>(i)));
	}

	// send store inbox
	sendInventoryItem(CONST_SLOT_STORE_INBOX, player->getStoreInbox()->getItem());

	// player light level
	sendCreatureLight(creature);

	// player vip list
	sendVIPEntries();

	// tiers for forge and market
	sendItemClasses();

	// opened containers
	player->openSavedContainers();
}

void Writer::write_add_marker(const Position& pos, uint8_t mark_type, std::string_view desc) {}

void Writer::write_add_tile_item(const Position& pos, uint32_t stackpos, const Item* item) {}

void Writer::write_basic_data() {}

void Writer::write_cancel_target() {}

void Writer::write_cancel_walk() {}

void Writer::write_challenge(uint32_t timestamp, uint8_t random_number) {}

void Writer::write_change_speed(const Creature* creature, uint32_t speed) {}

void Writer::write_channel(uint16_t id, std::string_view name, const std::map<uint32_t, Player*>* users,
                           const std::map<uint32_t, const Player*>* invited_users)
{}

void Writer::write_channel_event(uint16_t id, std::string_view player_name, ChannelEvent_t event) {}

void Writer::write_channel_message(std::string_view author, std::string_view text, SpeakClasses type, uint16_t channel)
{}

void Writer::write_channels_dialog() {}

void Writer::write_client_features() {}

void Writer::write_close_container(uint8_t cid) {}

void Writer::write_close_private(uint16_t channel_id) {}

void Writer::write_close_shop() {}

void Writer::write_close_trade() {}

void Writer::write_combat_analyzer(CombatType_t type, int32_t amount, DamageAnalyzerImpactType impact_type,
                                   std::string_view target)
{
	msg.add<uint8_t>(0xCC);
	msg.add<uint8_t>(impact_type);
	msg.add<uint32_t>(amount);

	switch (impact_type) {
		case RECEIVED:
			msg.add<uint8_t>(getClientDamageType(type));
			msg.add(target);
			break;

		case DEALT:
			msg.add<uint8_t>(getClientDamageType(type));
			break;

		default:
			break;
	}

	enqueue(msg.take());
}

void Writer::write_container(uint8_t cid, const Container* container, uint16_t first_index) {}

void Writer::write_create_private_channel(uint16_t channel_id, std::string_view channel_name) {}

void Writer::write_creature_health(const Creature* creature) {}

void Writer::write_creature_light(const Creature* creature) {}

void Writer::write_creature_outfit(const Creature* creature, const Outfit_t& outfit) {}

void Writer::write_creature_say(const Creature* creature, SpeakClasses type, std::string_view text, const Position* pos)
{}

void Writer::write_creature_shield(const Creature* creature) {}

void Writer::write_creature_skull(const Creature* creature) {}

void Writer::write_creature_square(const Creature* creature, SquareColor_t color) {}

void Writer::write_creature_turn(const Creature* creature, uint32_t stackpos) {}

void Writer::write_creature_walkthrough(const Creature* creature, bool walkthrough) {}

void Writer::write_distance_shoot(const Position& from, const Position& to, uint8_t type) {}

void Writer::write_empty_container(uint8_t cid) {}

void Writer::write_enter_world() {}

void Writer::write_experience_tracker(int64_t raw_exp, int64_t final_exp) {}

void Writer::write_fight_modes()
{
	msg.add<uint8_t>(0xA7);
	msg.add<uint8_t>(player->fightMode);
	msg.add<uint8_t>(player->chaseMode);
	msg.add<uint8_t>(player->secureMode);
	msg.add<uint8_t>(PVP_MODE_DOVE);
	enqueue(msg.take());
}

void Writer::write_fyi_box(std::string_view message) {}

void Writer::write_house_window(uint32_t id, std::string_view text)
{
	msg.add<uint8_t>(0x97);
	msg.add<uint8_t>(0x00);
	msg.add<uint32_t>(id);
	msg.add(text);
	enqueue(msg.take());
}

void Writer::write_icons(uint32_t icons) {}

void Writer::write_inventory_item(slots_t slot, const Item* item)
{
	if (item) {
		msg.add<uint8_t>(0x78);
		msg.add<uint8_t>(slot);
		msg.add(item);
	} else {
		msg.add<uint8_t>(0x79);
		msg.add<uint8_t>(slot);
	}

	enqueue(msg.take());
}

void Writer::write_item_classes() {}

void Writer::write_magic_effect(const Position& pos, uint8_t type) {}

void Writer::write_map_description(const Position& pos) {}

void Writer::write_market_accept_offer(const MarketOfferEx& offer) {}

void Writer::write_market_browse_item(uint16_t item_id, const MarketOfferList& buy_offers,
                                      const MarketOfferList& sell_offers)
{}

void Writer::write_market_browse_own_history(const HistoryMarketOfferList& buy_offers,
                                             const HistoryMarketOfferList& sell_offers)
{}

void Writer::write_market_browse_own_offers(const MarketOfferList& buy_offers, const MarketOfferList& sell_offers) {}

void Writer::write_market_cancel_offer(const MarketOfferEx& offer) {}

void Writer::write_market_enter() {}

void Writer::write_market_leave() {}

void Writer::write_modal_window(const ModalWindow& modal_window) {}

void Writer::write_move_creature(const Creature* creature, const Position& new_pos, int32_t new_stack_pos,
                                 const Position& old_pos, int32_t old_stack_pos, bool teleport)
{}

void Writer::write_open_private_channel(std::string_view receiver)
{
	msg.add<uint8_t>(0xAD);
	msg.add(receiver);
	enqueue(msg.take());
}

void Writer::write_outfit_window() {}

void Writer::write_pending_state_entered() {}

void Writer::write_ping() {}

void Writer::write_ping_back() {}

void Writer::write_podium_window(const Item* item) {}

void Writer::write_private_message(const Player* speaker, SpeakClasses type, std::string_view text) {}

void Writer::write_relogin_window(uint8_t unfair_fight_reduction) {}

void Writer::write_remove_container_item(uint8_t cid, uint16_t slot, const Item* last_item)
{
	msg.add<uint8_t>(0x72);
	msg.add<uint8_t>(cid);
	msg.add<uint16_t>(slot);

	if (last_item) {
		msg.add(last_item);
	} else {
		msg.add<uint16_t>(0x00);
	}

	enqueue(msg.take());
}

void Writer::write_remove_tile_creature(const Creature* creature, const Position& pos, uint32_t stackpos) {}

void Writer::write_remove_tile_thing(const Position& pos, uint32_t stackpos) {}

void Writer::write_resource_balance(const ResourceTypes_t resource_type, uint64_t amount) {}

void Writer::write_sale_item_list(const std::list<ShopInfo>& shop) {}

void Writer::write_session_end(SessionEndTypes_t reason) {}

void Writer::write_shop(Npc* npc, const ShopInfoList& item_list) {}

void Writer::write_skills() {}

void Writer::write_spell_cooldown(uint8_t spell_id, uint32_t time) {}

void Writer::write_spell_group_cooldown(SpellGroup_t group_id, uint32_t time) {}

void Writer::write_stats() {}

void Writer::write_store_balance() {}

void Writer::write_supply_used(const uint16_t client_id) {}

void Writer::write_text_message(const TextMessage& message) {}

void Writer::write_text_window(uint32_t id, Item* item, uint16_t maxlen, bool can_write)
{
	msg.add<uint8_t>(0x96);
	msg.add<uint32_t>(id);
	msg.add(item);

	if (can_write) {
		msg.add<uint16_t>(maxlen);
		msg.add(item->getText());
	} else {
		const auto& text = item->getText();
		msg.add<uint16_t>(text.size());
		msg.add(text);
	}

	const auto& writer = item->getWriter();
	if (!writer.empty()) {
		msg.add(writer);
	} else {
		msg.add<uint16_t>(0x00);
	}

	msg.add<uint8_t>(0x00);

	if (const auto writtenDate = item->getDate()) {
		msg.add(formatDateShort(writtenDate));
	} else {
		msg.add<uint16_t>(0x00);
	}

	enqueue(msg.take());
}

void Writer::write_text_window(uint32_t id, uint32_t item_id, std::string_view text)
{
	msg.add<uint8_t>(0x96);
	msg.add<uint32_t>(id);
	msg.add(item_id, 1);
	msg.add<uint16_t>(text.size());
	msg.add(text);
	msg.add<uint16_t>(0x00); // writer name
	msg.add<uint8_t>(0x00);  // "(traded)" byte
	msg.add<uint16_t>(0x00); // date
	enqueue(msg.take());
}

void Writer::write_to_channel(const Creature* creature, SpeakClasses type, std::string_view text, uint16_t channel_id)
{}

void Writer::write_tutorial(uint8_t tutorial_id) {}

void Writer::write_update_container_item(uint8_t cid, uint16_t slot, const Item* item)
{
	msg.add<uint8_t>(0x71);
	msg.add<uint8_t>(cid);
	msg.add<uint16_t>(slot);
	msg.add(item);
	enqueue(msg.take());
}

void Writer::write_update_creature_icons(const Creature* creature) {}

void Writer::write_update_tile(const Tile* tile, const Position& pos) {}

void Writer::write_update_tile_creature(const Position& pos, uint32_t stackpos, const Creature* creature) {}

void Writer::write_update_tile_item(const Position& pos, uint32_t stackpos, const Item* item) {}

void Writer::write_updated_vip_status(uint32_t guid, VipStatus_t status)
{
	msg.add<uint8_t>(0xD3);
	msg.add<uint32_t>(guid);
	msg.add<uint8_t>(status);
	enqueue(msg.take());
}

void Writer::write_use_item_cooldown(uint32_t time) {}

void Writer::write_vip(uint32_t guid, std::string_view name, std::string_view description, uint32_t icon, bool notify,
                       VipStatus_t status)
{
	msg.add<uint8_t>(0xD2);
	msg.add<uint32_t>(guid);
	msg.add(name);
	msg.add(description);
	msg.add<uint32_t>(std::min<uint32_t>(10, icon));
	msg.add<uint8_t>(notify ? 0x01 : 0x00);
	msg.add<uint8_t>(status);
	msg.add<uint8_t>(0x00);
	enqueue(msg.take());
}

void Writer::write_vip_entries()
{
	const auto& vipEntries = IOLoginData::getVIPEntries(player->getAccount());

	for (const VIPEntry& entry : vipEntries) {
		VipStatus_t vipStatus = VIPSTATUS_ONLINE;

		Player* vipPlayer = g_game.getPlayerByGUID(entry.guid);
		if (!vipPlayer || !player->canSeeCreature(vipPlayer)) {
			vipStatus = VIPSTATUS_OFFLINE;
		}

		write_vip(entry.guid, entry.name, entry.description, entry.icon, entry.notify, vipStatus);
	}
}

void Writer::write_items(std::map<uint32_t, uint32_t> inventory)
{
	msg.add<uint8_t>(0xF5);
	msg.add<uint16_t>(inventory.size() + 11);
	for (uint16_t i = 1; i <= 11; i++) {
		msg.add<uint16_t>(i); // slotId
		msg.add<uint8_t>(0);  // always 0
		msg.add<uint16_t>(1); // always 1
	}

	for (const auto& item : inventory) {
		msg.add<uint16_t>(Item::items[item.first].clientId); // item clientId
		msg.add<uint8_t>(0);                                 // always 0
		msg.add<uint16_t>(item.second);                      // count
	}

	enqueue(msg.take());
}

} // namespace tfs::game_server
