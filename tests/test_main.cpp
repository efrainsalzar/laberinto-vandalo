// Punto de entrada de los tests unitarios (doctest).
// Ejecutar con: scons tests=yes
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "version.h"

#include <string>

TEST_CASE("El entorno de tests funciona") {
	CHECK(std::string(LV_VERSION).size() > 0);
}
