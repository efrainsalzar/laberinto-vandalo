#pragma once

#include "game/usable_item.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

class Participant;

// La "mochila" de un participante: UNA ranura con un objeto usable (bomba o mina) y cuántos lleva.
// Componente hijo (composición, como WeaponComponent). Ver docs/ARQUITECTURA.md §28.
//
// - give(item): lo llama GiveItemEffect al recoger el objeto. Mismo objeto = +1 (hasta max_carry);
//   otro objeto = lo reemplaza (como el arma especial).
// - update_use(participant, down): lo llama Participant cada frame con ParticipantCommand.use_item; usa UNO
//   al apretar (no repite si se mantiene). Lo deja en el piso con ProjectileSystem::place_explosive.
// - Al perder la vida se pierde lo que llevaba.
// - Señal item_changed(item, count) para la tarjeta.
class ItemComponent : public Node {
	GDCLASS(ItemComponent, Node)

public:
	bool can_accept(const Ref<UsableItem> &p_item) const;
	bool give(const Ref<UsableItem> &p_item, int p_count = 1);
	bool update_use(Participant *p_participant, bool p_down);
	void clear();

	Ref<UsableItem> get_item() const { return item_; }
	int get_count() const { return count_; }

protected:
	static void _bind_methods();

private:
	Ref<UsableItem> item_;
	int count_ = 0;
	bool was_down_ = false;

	void emit_changed();
};
