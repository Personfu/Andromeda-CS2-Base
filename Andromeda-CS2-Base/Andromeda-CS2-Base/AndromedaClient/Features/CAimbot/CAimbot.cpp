#include "CAimbot.hpp"

#include <cmath>

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>
#include <CS2/SDK/Math/Math.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>

#include <GameClient/CEntityCache/CEntityCache.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Bones.hpp>
#include <GameClient/CL_VisibleCheck.hpp>
#include <GameClient/CL_Bypass.hpp>

#include <AndromedaClient/Settings/Settings.hpp>

static CAimbot g_CAimbot{};

// ------------------------------------------------------------
// CS2 bone name table — indexed by Settings::Aimbot::BoneIndex
// ------------------------------------------------------------
static constexpr const char* s_AimBoneNames[] =
{
	"head_0" ,  // 0 - Head
	"neck_0" ,  // 1 - Neck
	"spine_1" , // 2 - Upper body
};

static constexpr int s_AimBoneCount = static_cast<int>( std::size( s_AimBoneNames ) );

// Internal result type so we only walk the entity cache once per tick
struct BestTarget_t
{
	C_CSPlayerPawn* pPawn = nullptr;
	Vector3         BonePos;
};

// ------------------------------------------------------------

auto CAimbot::IsValidTarget( C_CSPlayerPawn* pPawn ) -> bool
{
	if ( !pPawn || !pPawn->IsPlayerPawn() )
		return false;

	if ( !pPawn->IsAlive() )
		return false;

	auto* pLocalPawn = GetCL_Players()->GetLocalPlayerPawn();
	if ( !pLocalPawn )
		return false;

	// Never aim at ourselves
	if ( pPawn == pLocalPawn )
		return false;

	// Team-fire guard
	if ( Settings::Aimbot::OnlyEnemy )
	{
		if ( pPawn->m_iTeamNum() == pLocalPawn->m_iTeamNum() )
			return false;
	}

	// Trace-based visibility gate
	if ( Settings::Aimbot::OnlyVisible )
	{
		if ( !GetCL_VisibleCheck()->IsPlayerPawnVisible( pPawn ) )
			return false;
	}

	return true;
}

auto CAimbot::GetBonePosition( C_CSPlayerPawn* pPawn ) -> Vector3
{
	int idx = Settings::Aimbot::BoneIndex;

	if ( idx < 0 || idx >= s_AimBoneCount )
		idx = 0;

	return GetCL_Bones()->GetBonePositionByName( pPawn , s_AimBoneNames[idx] );
}

// Returns angular distance in degrees between viewAngles and a world-space position
auto CAimbot::GetFovToTarget( const QAngle& viewAngles , const Vector3& bonePos ) -> float
{
	const auto localEye = GetCL_Players()->GetLocalEyeOrigin();
	const auto aimAngle = Math::CalcAngle( localEye , bonePos );

	const float dx = Math::AngleNormalize( aimAngle.m_x - viewAngles.m_x );
	const float dy = Math::AngleNormalize( aimAngle.m_y - viewAngles.m_y );

	return std::sqrtf( dx * dx + dy * dy );
}

// Walks entity cache under lock, returns the best target + cached bone position.
// Doing both operations under the same lock prevents a TOCTOU race if an entity
// is removed between finding the target and sampling its bone.
auto CAimbot::FindBestTarget( const QAngle& viewAngles ) -> BestTarget_t
{
	const auto& CachedVec = GetEntityCache()->GetCachedEntity();
	std::scoped_lock Lock( GetEntityCache()->GetLock() );

	BestTarget_t best{};
	float fBestFov = Settings::Aimbot::FOV; // only accept targets within configured FOV

	for ( const auto& CachedEntity : *CachedVec )
	{
		if ( CachedEntity.m_Type != CachedEntity_t::PLAYER_CONTROLLER )
			continue;

		auto* pEntity = CachedEntity.m_Handle.Get();
		if ( !pEntity )
			continue;

		// Guard against stale handles
		if ( pEntity->pEntityIdentity()->Handle() != CachedEntity.m_Handle )
			continue;

		auto* pController = reinterpret_cast<CCSPlayerController*>( pEntity );
		if ( !pController->m_bPawnIsAlive() )
			continue;

		auto* pPawn = pController->m_hPawn().Get<C_CSPlayerPawn>();
		if ( !IsValidTarget( pPawn ) )
			continue;

		const auto bonePos = GetBonePosition( pPawn );
		if ( bonePos.IsZero() )
			continue;

		const float fov = GetFovToTarget( viewAngles , bonePos );
		if ( fov < fBestFov )
		{
			fBestFov     = fov;
			best.pPawn   = pPawn;
			best.BonePos = bonePos;
		}
	}

	return best;
}

// ------------------------------------------------------------
// OnCreateMove — called every CS2 input tick via Hook_CreateMove
// Uses CL_Bypass::SetViewAngles so angle writes go through the
// protobuf CRC spoof path — safe to use alongside the base bypass.
// ------------------------------------------------------------
auto CAimbot::OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void
{
	if ( !Settings::Aimbot::Active )
		return;

	if ( !SDK::Interfaces::EngineToClient()->IsInGame() )
		return;

	if ( !GetCL_Players()->IsLocalPlayerAlive() )
		return;

	// Read current CS2 view angles from the input-moves structure
	const auto& vecView  = pInput->m_pInputMoves()->m_vecViewAngles();
	const QAngle viewAngles( vecView.m_x , vecView.m_y , 0.f );

	// Aim key — default Left Alt (VK_LMENU = 0xA4)
	const bool bKeyHeld = ( GetAsyncKeyState( Settings::Aimbot::HoldKey ) & 0x8000 ) != 0;

	// TriggerBot can fire without the aim key held
	const bool bTrigger = Settings::Aimbot::TriggerBot;

	if ( !bKeyHeld && !bTrigger )
		return;

	const auto target = FindBestTarget( viewAngles );

	if ( !target.pPawn || target.BonePos.IsZero() )
		return;

	const auto localEye = GetCL_Players()->GetLocalEyeOrigin();
	auto aimAngle       = Math::CalcAngle( localEye , target.BonePos );

	// TriggerBot — auto-fire when we're already pointed at the target
	if ( bTrigger )
	{
		const float trigFov = GetFovToTarget( viewAngles , target.BonePos );
		if ( trigFov < Settings::Aimbot::TriggerFOV )
			GetCL_Bypass()->SetAttack( pUserCmd , true );
	}

	// Smooth aim — only redirect angles when hold key is pressed
	if ( bKeyHeld )
	{
		QAngle smoothed;
		// SmoothAngles: Smoothing=1 → instant snap; higher = softer interpolation
		Math::SmoothAngles( viewAngles , aimAngle , smoothed , Settings::Aimbot::Smoothing );

		// SetViewAngles writes through the CRC-bypass path — required in CS2
		GetCL_Bypass()->SetViewAngles( &smoothed , pInput , pUserCmd );
	}
}

auto GetAimbot() -> CAimbot*
{
	return &g_CAimbot;
}
