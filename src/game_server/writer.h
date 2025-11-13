#pragma once

#include "connection.h"
#include "outgoing_message.h"

#include <memory>

namespace tfs::game_server {

class Writer
{
public:
	constexpr Writer() = default;
	virtual ~Writer() = default;

	void write_challenge(uint32_t timestamp, uint8_t random_number);
	void write_add_container_item(uint8_t cid, uint16_t slot, const Item* item);
	void write_add_creature(const Creature* creature, const Position& pos, int32_t stackpos,
	                        MagicEffectClasses magic_effect = CONST_ME_NONE);
	void write_add_marker(const Position& pos, uint8_t mark_type, const std::string& desc);
	void write_add_tile_item(const Position& pos, uint32_t stackpos, const Item* item);
	void write_basic_data();
	void write_cancel_target();
	void write_cancel_walk();
	void write_change_speed(const Creature* creature, uint32_t speed);
	void write_channel(uint16_t id, const std::string& name, const std::map<uint32_t, Player*>* users,
	                   const std::map<uint32_t, const Player*>* invited_users);
	void write_channel_event(uint16_t id, const std::string& player_name, ChannelEvent_t event);
	void write_channel_message(const std::string& author, const std::string& text, SpeakClasses type, uint16_t channel);
	void write_channels_dialog();
	void write_client_features();
	void write_close_container(uint8_t cid);
	void write_close_private(uint16_t channel_id);
	void write_close_shop();
	void write_close_trade();
	void write_combat_analyzer(CombatType_t type, int32_t amount, DamageAnalyzerImpactType impact_type,
	                           const std::string& target);
	void write_container(uint8_t cid, const Container* container, uint16_t first_index);
	void write_create_private_channel(uint16_t channel_id, const std::string& channel_name);
	void write_creature_health(const Creature* creature);
	void write_creature_light(const Creature* creature);
	void write_creature_outfit(const Creature* creature, const Outfit_t& outfit);
	void write_creature_say(const Creature* creature, SpeakClasses type, const std::string& text,
	                        const Position* pos = nullptr);
	void write_creature_shield(const Creature* creature);
	void write_creature_skull(const Creature* creature);
	void write_creature_square(const Creature* creature, SquareColor_t color);
	void write_creature_turn(const Creature* creature, uint32_t stackpos);
	void write_creature_walkthrough(const Creature* creature, bool walkthrough);
	void write_distance_shoot(const Position& from, const Position& to, uint8_t type);
	void write_empty_container(uint8_t cid);
	void write_enter_world();
	void write_experience_tracker(int64_t raw_exp, int64_t final_exp);
	void write_fight_modes();
	void write_fyi_box(const std::string& message);
	void write_house_window(uint32_t id, const std::string& text);
	void write_icons(uint32_t icons);
	void write_inventory_item(slots_t slot, const Item* item);
	void write_item_classes();
	void write_magic_effect(const Position& pos, uint8_t type);
	void write_map_description(const Position& pos);
	void write_market_accept_offer(const MarketOfferEx& offer);
	void write_market_browse_item(uint16_t item_id, const MarketOfferList& buy_offers,
	                              const MarketOfferList& sell_offers);
	void write_market_browse_own_history(const HistoryMarketOfferList& buy_offers,
	                                     const HistoryMarketOfferList& sell_offers);
	void write_market_browse_own_offers(const MarketOfferList& buy_offers, const MarketOfferList& sell_offers);
	void write_market_cancel_offer(const MarketOfferEx& offer);
	void write_market_enter();
	void write_market_leave();
	void write_modal_window(const ModalWindow& modal_window);
	void write_move_creature(const Creature* creature, const Position& new_pos, int32_t new_stack_pos,
	                         const Position& old_pos, int32_t old_stack_pos, bool teleport);
	void write_open_private_channel(const std::string& receiver);
	void write_outfit_window();
	void write_pending_state_entered();
	void write_ping();
	void write_ping_back();
	void write_podium_window(const Item* item);
	void write_private_message(const Player* speaker, SpeakClasses type, const std::string& text);
	void write_relogin_window(uint8_t unfair_fight_reduction);
	void write_remove_container_item(uint8_t cid, uint16_t slot, const Item* last_item);
	void write_remove_tile_creature(const Creature* creature, const Position& pos, uint32_t stackpos);
	void write_remove_tile_thing(const Position& pos, uint32_t stackpos);
	void write_resource_balance(const ResourceTypes_t resource_type, uint64_t amount);
	void write_sale_item_list(const std::list<ShopInfo>& shop);
	void write_session_end(SessionEndTypes_t reason);
	void write_shop(Npc* npc, const ShopInfoList& item_list);
	void write_skills();
	void write_spell_cooldown(uint8_t spell_id, uint32_t time);
	void write_spell_group_cooldown(SpellGroup_t group_id, uint32_t time);
	void write_stats();
	void write_store_balance();
	void write_supply_used(const uint16_t client_id);
	void write_text_message(const TextMessage& message);
	void write_text_window(uint32_t id, Item* item, uint16_t maxlen, bool can_write);
	void write_text_window(uint32_t id, uint32_t item_id, const std::string& text);
	void write_to_channel(const Creature* creature, SpeakClasses type, const std::string& text, uint16_t channel_id);
	void write_tutorial(uint8_t tutorial_id);
	void write_update_container_item(uint8_t cid, uint16_t slot, const Item* item);
	void write_update_creature_icons(const Creature* creature);
	void write_update_tile(const Tile* tile, const Position& pos);
	void write_update_tile_creature(const Position& pos, uint32_t stackpos, const Creature* creature);
	void write_update_tile_item(const Position& pos, uint32_t stackpos, const Item* item);
	void write_updated_vip_status(uint32_t guid, VipStatus_t new_status);
	void write_use_item_cooldown(uint32_t time);
	void write_vip(uint32_t guid, const std::string& name, const std::string& description, uint32_t icon, bool notify,
	               VipStatus_t status);
	void write_vip_entries();
	void write_items();
};

} // namespace tfs::game_server
