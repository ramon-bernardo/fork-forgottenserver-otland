#include "outgoing_message.h"

#include "../container.h"
#include "../podium.h"

#include <boost/locale.hpp>
#include <string>

namespace locale = boost::locale;

namespace tfs::game_server {

template <typename T>
void OutgoingMessage::add(T value)
{
	std::memcpy(buffer.data() + length, &value, sizeof(T));
	length += sizeof(T);
}

template <>
void OutgoingMessage::add(std::string_view s)
{
	const auto str = locale::conv::from_utf<char>(s.data(), s.data() + s.size(), "ISO-8859-1", locale::conv::skip);

	const auto n = str.size();
	add<uint16_t>(n);

	std::memcpy(buffer.data() + length, str.data(), n);
	length += n;
}

template <>
void OutgoingMessage::add(Position position)
{
	add<uint16_t>(position.x);
	add<uint16_t>(position.y);
	add<uint8_t>(position.z);
}

template <>
void OutgoingMessage::add(const Item* item)
{
	const auto& it = Item::items[item->getID()];

	add<uint16_t>(it.clientId);

	if (it.stackable) {
		add<uint8_t>(std::min<uint16_t>(0xFF, item->getItemCount()));
	} else if (it.isSplash() || it.isFluidContainer()) {
		add<uint8_t>(fluidMap[item->getFluidType() & 7]);
	} else if (it.classification > 0) {
		add<uint8_t>(0x00);
	}

	if (it.showClientCharges) {
		add<uint32_t>(item->getCharges());
		add<uint8_t>(0);
	} else if (it.showClientDuration) {
		add<uint32_t>(item->getDuration() / 1000);
		add<uint8_t>(0);
	}

	if (it.isContainer()) {
		add<uint8_t>(0x00);

		const Container* container = item->getContainer();
		if (container && it.weaponType == WEAPON_QUIVER) {
			add<uint8_t>(0x01);
			add<uint32_t>(container->getAmmoCount());
		} else {
			add<uint8_t>(0x00);
		}
	}

	if (it.isPodium()) {
		const Podium* podium = item->getPodium();
		const Outfit_t& outfit = podium->getOutfit();

		if (podium->hasFlag(PODIUM_SHOW_OUTFIT)) {
			add<uint16_t>(outfit.lookType);
			if (outfit.lookType != 0) {
				add<uint8_t>(outfit.lookHead);
				add<uint8_t>(outfit.lookBody);
				add<uint8_t>(outfit.lookLegs);
				add<uint8_t>(outfit.lookFeet);
				add<uint8_t>(outfit.lookAddons);
			}
		} else {
			add<uint16_t>(0);
		}

		if (podium->hasFlag(PODIUM_SHOW_MOUNT)) {
			add<uint16_t>(outfit.lookMount);
			if (outfit.lookMount != 0) {
				add<uint8_t>(outfit.lookMountHead);
				add<uint8_t>(outfit.lookMountBody);
				add<uint8_t>(outfit.lookMountLegs);
				add<uint8_t>(outfit.lookMountFeet);
			}
		} else {
			add<uint16_t>(0);
		}

		add<uint8_t>(podium->getDirection());
		add<uint8_t>(podium->hasFlag(PODIUM_SHOW_PLATFORM) ? 0x01 : 0x00);
	}
}

} // namespace tfs::game_server
