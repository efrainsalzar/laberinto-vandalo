shader_type canvas_item;
// Desintegración por "píxeles" (solo vista): el dibujo se divide en cuadraditos de `pixel_size` px y cada uno
// desaparece en su momento según un valor al azar fijo. Justo antes de desaparecer, cada cuadradito brilla con
// `edge_color` (el borde que "se está quemando"). progress: 0 = entero, 1 = desaparecido.
// Funciona con dibujos hechos con draw_circle/draw_line (usa la posición local, no la UV).
// Lo usa scripts/participant/participant_view.gd. Ver docs/arquitectura/22-armas-disparo.md.

uniform float progress : hint_range(0.0, 1.0) = 0.0;
uniform float pixel_size = 2.5;
uniform float edge_width = 0.12;
uniform float body_radius = 11.0; // para deshacerse de afuera hacia adentro
uniform vec4 edge_color : source_color = vec4(1.0);

varying vec2 local_pos;

void vertex() {
	local_pos = VERTEX;
}

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void fragment() {
	// (En fragment() no se permite "return": todo va dentro del if.)
	if (progress > 0.0) {
		float n = hash(floor(local_pos / pixel_size));
		// Los de afuera se van antes que los del centro (se deshace de afuera hacia adentro).
		float radial = clamp(length(local_pos) / max(body_radius, 1.0), 0.0, 1.0);
		float t = mix(n, 1.0 - radial, 0.35);
		float cut = progress * (1.0 + edge_width);
		if (t < cut - edge_width) {
			discard;
		}
		if (t < cut) {
			COLOR = vec4(edge_color.rgb, COLOR.a);
		}
	}
}
