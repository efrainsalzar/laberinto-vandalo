#include "core/shields.h"

#include <algorithm>
#include <cmath>

namespace lv::core {

ShieldOutcome BubbleShield::filter(const DamageInfo &p_info, const ShieldContext &p_context) {
	(void)p_context;
	if (popped_ || p_info.amount <= 0.0f) {
		return ShieldOutcome::Pass;
	}
	if (p_info.kind == DamageKind::Beam) {
		beam_budget_ -= p_info.amount;
		popped_ = beam_budget_ <= 0.0f;
		return ShieldOutcome::Absorbed;
	}
	popped_ = true; // un golpe, sea cual sea, y se rompe
	return ShieldOutcome::Absorbed;
}

FrontalShield::FrontalShield(float p_duration, float p_half_angle_deg) :
		Shield(p_duration) {
	cos_half_ = std::cos(p_half_angle_deg * 3.14159265f / 180.0f);
}

ShieldOutcome FrontalShield::filter(const DamageInfo &p_info, const ShieldContext &p_context) {
	const float dir_len = std::sqrt(p_info.dir_x * p_info.dir_x + p_info.dir_y * p_info.dir_y);
	const float aim_len = std::sqrt(p_context.aim_x * p_context.aim_x + p_context.aim_y * p_context.aim_y);
	if (dir_len <= 0.0001f || aim_len <= 0.0001f) {
		return ShieldOutcome::Pass; // sin dirección (explosión encima) no hay "delante"
	}
	// El golpe viaja en `dir`; viene DESDE -dir. Está delante si -dir apunta hacia donde mira el portador.
	const float dot = -(p_info.dir_x * p_context.aim_x + p_info.dir_y * p_context.aim_y) / (dir_len * aim_len);
	return dot >= cos_half_ ? ShieldOutcome::Absorbed : ShieldOutcome::Pass;
}

ShieldOutcome MirrorShield::filter(const DamageInfo &p_info, const ShieldContext &p_context) {
	(void)p_context;
	if (p_info.kind == DamageKind::Explosion) {
		return ShieldOutcome::Pass; // la onda expansiva no se puede devolver
	}
	return p_info.reflected ? ShieldOutcome::Absorbed : ShieldOutcome::Reflected;
}

void ShieldChain::add(std::unique_ptr<Shield> p_shield) {
	if (p_shield == nullptr) {
		return;
	}
	for (std::unique_ptr<Shield> &link : links_) {
		if (link->kind() == p_shield->kind()) {
			link = std::move(p_shield); // mismo tipo: el nuevo reemplaza al viejo (tiempo lleno, burbuja entera)
			return;
		}
	}
	links_.push_back(std::move(p_shield));
	std::stable_sort(links_.begin(), links_.end(), [](const std::unique_ptr<Shield> &a, const std::unique_ptr<Shield> &b) {
		return static_cast<int>(a->kind()) < static_cast<int>(b->kind());
	});
}

ShieldResult ShieldChain::filter(const DamageInfo &p_info, const ShieldContext &p_context) {
	ShieldResult result;
	for (std::unique_ptr<Shield> &link : links_) {
		if (link->is_spent()) {
			continue;
		}
		const ShieldOutcome outcome = link->filter(p_info, p_context);
		if (outcome != ShieldOutcome::Pass) {
			result.outcome = outcome;
			result.by = link->kind();
			return result;
		}
	}
	return result;
}

void ShieldChain::remove_spent(std::vector<ShieldKind> &r_ended) {
	for (auto it = links_.begin(); it != links_.end();) {
		if ((*it)->is_spent()) {
			r_ended.push_back((*it)->kind());
			it = links_.erase(it);
		} else {
			++it;
		}
	}
}

void ShieldChain::tick(float p_delta, std::vector<ShieldKind> &r_ended) {
	for (std::unique_ptr<Shield> &link : links_) {
		link->tick(p_delta);
	}
	remove_spent(r_ended);
}

const Shield *ShieldChain::find(ShieldKind p_kind) const {
	for (const std::unique_ptr<Shield> &link : links_) {
		if (link->kind() == p_kind) {
			return link.get();
		}
	}
	return nullptr;
}

} // namespace lv::core
