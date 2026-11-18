#include "game/item_component.h"

#include "game/participant.h"
#include "game/projectile_system.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>

void ItemComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("can_accept", "item"), &ItemComponent::can_accept);
	ClassDB::bind_method(D_METHOD("give", "item", "count"), &ItemComponent::give, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("clear"), &ItemComponent::clear);
	ClassDB::bind_method(D_METHOD("get_item"), &ItemComponent::get_item);
	ClassDB::bind_method(D_METHOD("get_count"), &ItemComponent::get_count);

	ADD_SIGNAL(MethodInfo("item_changed", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "UsableItem"), PropertyInfo(Variant::INT, "count")));
	ADD_SIGNAL(MethodInfo("item_used", PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "UsableItem")));
}

bool ItemComponent::can_accept(const Ref<UsableItem> &p_item) const {
	if (p_item.is_null()) {
		return false;
	}
	// Lleno del mismo: no lo recoge (queda en el piso). Otro objeto siempre se puede (reemplaza).
	return !(item_ == p_item && count_ >= p_item->get_max_carry());
}

bool ItemComponent::give(const Ref<UsableItem> &p_item, int p_count) {
	if (!can_accept(p_item) || p_count <= 0) {
		return false;
	}
	if (item_ != p_item) {
		item_ = p_item;
		count_ = 0;
	}
	count_ = std::min(count_ + p_count, p_item->get_max_carry());
	emit_changed();
	return true;
}

bool ItemComponent::update_use(Participant *p_participant, bool p_down) {
	const bool pressed = p_down && !was_down_;
	was_down_ = p_down;
	if (!pressed || item_.is_null() || count_ <= 0 || p_participant == nullptr) {
		return false;
	}
	ProjectileSystem *system = ProjectileSystem::find(get_tree());
	if (system == nullptr) {
		return false;
	}
	system->place_explosive(p_participant, item_);
	const Ref<UsableItem> used = item_;
	count_--;
	if (count_ <= 0) {
		item_.unref();
		count_ = 0;
	}
	emit_signal("item_used", used);
	emit_changed();
	return true;
}

void ItemComponent::clear() {
	if (item_.is_null() && count_ == 0) {
		return;
	}
	item_.unref();
	count_ = 0;
	emit_changed();
}

void ItemComponent::emit_changed() {
	emit_signal("item_changed", item_, count_);
}
