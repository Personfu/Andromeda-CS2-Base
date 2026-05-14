#include "Hook_DrawGlow.hpp"

#include <intrin.h>

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>

#include <AndromedaClient/Features/CVisual/CVisual.hpp>

auto Hook_DrawGlow( CGlowProperty* pCGlowProperty ) -> void*
{
	auto pResult = DrawGlow_o( pCGlowProperty );

	if ( SDK::Interfaces::EngineToClient()->IsInGame() )
		GetVisual()->OnDrawGlow( pCGlowProperty );

	return pResult;
}
